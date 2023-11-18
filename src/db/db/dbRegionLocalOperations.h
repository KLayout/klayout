
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


#ifndef HDR_dbRegionLocalOperations
#define HDR_dbRegionLocalOperations

#include "dbCommon.h"
#include "dbEdgePairRelations.h"
#include "dbLocalOperation.h"
#include "dbLocalOperationUtils.h"
#include "dbEdgeProcessor.h"
#include "dbRegionCheckUtils.h"
#include "dbPropertyConstraint.h"
#include "dbLayoutUtils.h"

#include <vector>
#include <unordered_set>

namespace db
{

/**
 *  @brief Specifies an error filter on rectangular shapes
 *
 *  There are actually more combinations possible. The bit pattern of the enum value consists
 *  of groups of 4 bits each specifying an allowed pattern. Rotation is implicit, so it's just
 *  required to give on incarnation.
 *
 *  For example: 0x153 would be one- or two-sided.
 *
 *  The bitmaps are choosen such that they can be or-combined.
 */
enum RectFilter
{
  /**
   *  @brief No filter
   */
  NoRectFilter = 0,

  /**
   *  @brief Allow errors on one side
   */
  OneSideAllowed = 0x1,

  /**
   *  @brief Allow errors on two sides (not specified which)
   */
  TwoSidesAllowed = 0x530,

  /**
   *  @brief Allow errors on two sides ("L" configuration)
   */
  TwoConnectedSidesAllowed = 0x30,

  /**
   *  @brief Allow errors on two opposite sides
   */
  TwoOppositeSidesAllowed = 0x500,

  /**
   *  @brief Allow errors on three sides
   */
  ThreeSidesAllowed = 0x7000,

  /**
   *  @brief Allow errors when on all sides
   */
  FourSidesAllowed = 0xf0000
};

/**
 *  @brief Specifies an error filter for opposite errors
 */
enum OppositeFilter
{
  /**
   *  @brief No filter
   */
  NoOppositeFilter,

  /**
   *  @brief Only errors appearing on opposite sides of a figure will be reported
   */
  OnlyOpposite,

  /**
   *  @brief Only errors NOT appearing on opposite sides of a figure will be reported
   */
  NotOpposite
};

/**
 *  @brief A structure holding the options for the region checks (space, width, ...)
 */
struct DB_PUBLIC RegionCheckOptions
{
  typedef db::coord_traits<db::Coord>::distance_type distance_type;

  /**
   *  @brief Constructor
   */
  RegionCheckOptions (bool _whole_edges = false,
                      metrics_type _metrics = db::Euclidian,
                      double _ignore_angle = 90,
                      distance_type _min_projection = 0,
                      distance_type _max_projection = std::numeric_limits<distance_type>::max (),
                      bool _shielded = true,
                      OppositeFilter _opposite_filter = NoOppositeFilter,
                      RectFilter _rect_filter = NoRectFilter,
                      bool _negative = false,
                      PropertyConstraint _prop_constraint = IgnoreProperties)
    : whole_edges (_whole_edges),
      metrics (_metrics),
      ignore_angle (_ignore_angle),
      min_projection (_min_projection),
      max_projection (_max_projection),
      shielded (_shielded),
      opposite_filter (_opposite_filter),
      rect_filter (_rect_filter),
      negative (_negative),
      prop_constraint (_prop_constraint)
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

  /**
   *  @brief Specifies shielding
   *
   *  Set this option to false to disable shielding. By default, shielding is on.
   */
  bool shielded;

  /**
   *  @brief Specifies the opposite filter
   */
  OppositeFilter opposite_filter;

  /**
   *  @brief Specifies a filter for error markers on rectangular shapes
   */
  RectFilter rect_filter;

  /**
   *  @brief Specifies whether to produce negative output
   */
  bool negative;

  /**
   *  @brief Specifies a property constraint - e.g. checking only shapes with the same properties
   */
  PropertyConstraint prop_constraint;

  /**
   *  @brief Gets a value indicating whether merged primary input is required
   */
  bool needs_merged () const
  {
    return negative
             || rect_filter != NoRectFilter
             || opposite_filter != NoOppositeFilter
             || max_projection != std::numeric_limits<distance_type>::max ()
             || min_projection != 0
             || whole_edges;
  }
};

template <class TS, class TI>
class check_local_operation_base
{
public:
  check_local_operation_base (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options);

protected:
  EdgeRelationFilter m_check;
  bool m_different_polygons;
  bool m_is_merged;
  bool m_has_other;
  bool m_other_is_merged;
  db::RegionCheckOptions m_options;

  void compute_results (db::Layout *layout, db::Cell *subject_cell, const std::vector<const TS *> &subjects, const std::set<const TI *> &intruders, std::unordered_set<db::EdgePair> &result, std::unordered_set<db::EdgePair> &intra_polygon_result, const LocalProcessorBase *proc) const;
  void apply_opposite_filter (const std::vector<const TS *> &subjects, std::unordered_set<db::EdgePair> &result, std::unordered_set<db::EdgePair> &intra_polygon_result) const;
  void apply_rectangle_filter (const std::vector<const TS *> &subjects, std::unordered_set<db::EdgePair> &result) const;
};

template <class TS, class TI>
class check_local_operation
  : public local_operation<TS, TI, db::EdgePair>, public check_local_operation_base<TS, TI>
{
public:
  check_local_operation (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options);

  virtual db::Coord dist () const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const;

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase * /*proc*/) const;

  virtual const db::TransformationReducer *vars () const { return &m_vars; }

private:
  db::MagnificationReducer m_vars;
};

template <class TS, class TI>
class check_local_operation_with_properties
  : public local_operation<db::object_with_properties<TS>, db::object_with_properties<TI>, db::EdgePairWithProperties>, public check_local_operation_base<TS, TI>
{
public:
  check_local_operation_with_properties (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options, db::PropertiesRepository *target_pr, const db::PropertiesRepository *subject_pr, const db::PropertiesRepository *intruder_pr);

  virtual db::Coord dist () const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const;

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::EdgePairWithProperties> > &results, const db::LocalProcessorBase * /*proc*/) const;

  virtual const db::TransformationReducer *vars () const { return &m_vars; }

private:
  mutable db::PropertyMapper m_pms, m_pmi;
  db::MagnificationReducer m_vars;
};

typedef check_local_operation<db::PolygonRef, db::PolygonRef> CheckLocalOperation;

enum InteractingOutputMode {
  None = 0, Positive = 1, Negative = 2, PositiveAndNegative = 3
};

template <class TS, class TI, class TR>
class interacting_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_local_operation (int mode, bool touching, InteractingOutputMode output_mode, size_t min_count, size_t max_count, bool other_is_merged);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  int m_mode;
  bool m_touching;
  InteractingOutputMode m_output_mode;
  size_t m_min_count, m_max_count;
  bool m_other_is_merged;
};

typedef interacting_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> InteractingLocalOperation;

template <class TS, class TI, class TR>
class pull_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_local_operation (int mode, bool touching);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  int m_mode;
  bool m_touching;
};

typedef pull_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> PullLocalOperation;

template <class TS, class TI, class TR>
class interacting_with_edge_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_with_edge_local_operation (InteractingOutputMode output_mode, size_t min_count, size_t max_count, bool other_is_merged);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  InteractingOutputMode m_output_mode;
  size_t m_min_count, m_max_count;
  bool m_other_is_merged;
};

typedef interacting_with_edge_local_operation<db::PolygonRef, db::Edge, db::PolygonRef> InteractingWithEdgeLocalOperation;

template <class TS, class TI, class TR>
class pull_with_edge_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_with_edge_local_operation ();

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

typedef pull_with_edge_local_operation<db::PolygonRef, db::Edge, db::Edge> PullWithEdgeLocalOperation;

template <class TS, class TI, class TR>
class interacting_with_text_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_with_text_local_operation (InteractingOutputMode output_mode, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  InteractingOutputMode m_output_mode;
  size_t m_min_count, m_max_count;
};

typedef interacting_with_text_local_operation<db::PolygonRef, db::TextRef, db::PolygonRef> InteractingWithTextLocalOperation;

template <class TS, class TI, class TR>
class pull_with_text_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_with_text_local_operation ();

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

typedef pull_with_text_local_operation<db::PolygonRef, db::TextRef, db::TextRef> PullWithTextLocalOperation;

template <class TS, class TI, class TR>
class contained_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  contained_local_operation (InteractingOutputMode output_mode);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  InteractingOutputMode m_output_mode;
};

typedef contained_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> ContainedLocalOperation;
//  the implementation is type-agnostic and can be used for edges too
typedef contained_local_operation<db::Edge, db::Edge, db::Edge> ContainedEdgesLocalOperation;

/**
 *  @brief Implements a boolean AND or NOT operation
 */

template <class TS, class TI, class TR>
class DB_PUBLIC bool_and_or_not_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  bool_and_or_not_local_operation (bool is_and);

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &result, const db::LocalProcessorBase *proc) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_is_and;
};

typedef bool_and_or_not_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> BoolAndOrNotLocalOperation;

/**
 *  @brief Implements a boolean AND or NOT operation with property handling
 */
template <class TS, class TI, class TR>
class DB_PUBLIC bool_and_or_not_local_operation_with_properties
  : public local_operation<db::object_with_properties<TS>, db::object_with_properties<TI>, db::object_with_properties<TR> >
{
public:
  bool_and_or_not_local_operation_with_properties (bool is_and, db::PropertiesRepository *target_pr, const db::PropertiesRepository *subject_pr, const db::PropertiesRepository *intruder_pr, db::PropertyConstraint property_constraint);

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &result, const db::LocalProcessorBase *proc) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_is_and;
  db::PropertyConstraint m_property_constraint;
  mutable db::PropertyMapper m_pms;
  mutable db::PropertyMapper m_pmi;
};

typedef bool_and_or_not_local_operation_with_properties<db::PolygonRef, db::PolygonRef, db::PolygonRef> BoolAndOrNotLocalOperationWithProperties;

/**
 *  @brief Implements a boolean AND plus NOT operation
 *
 *  This processor delivers two outputs: the first one having the AND result, the second
 *  one having the NOT result.
 */

template <class TS, class TI, class TR>
class DB_PUBLIC two_bool_and_not_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  two_bool_and_not_local_operation ();

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &result, const db::LocalProcessorBase *proc) const;
  virtual std::string description () const;
};

typedef two_bool_and_not_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> TwoBoolAndNotLocalOperation;

/**
 *  @brief Implements a boolean AND plus NOT operation
 *
 *  This processor delivers two outputs: the first one having the AND result, the second
 *  one having the NOT result.
 */
template <class TS, class TI, class TR>
class DB_PUBLIC two_bool_and_not_local_operation_with_properties
  : public local_operation<db::object_with_properties<TS>, db::object_with_properties<TI>, db::object_with_properties<TR> >
{
public:
  two_bool_and_not_local_operation_with_properties (db::PropertiesRepository *target1_pr, db::PropertiesRepository *target2_pr, const db::PropertiesRepository *subject_pr, const db::PropertiesRepository *intruder_pr, db::PropertyConstraint property_constraint);

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &result, const db::LocalProcessorBase *proc) const;
  virtual std::string description () const;

private:
  db::PropertyConstraint m_property_constraint;
  mutable db::PropertyMapper m_pms, m_pmi, m_pm12;
};

typedef two_bool_and_not_local_operation_with_properties<db::PolygonRef, db::PolygonRef, db::PolygonRef> TwoBoolAndNotLocalOperationWithProperties;

/**
 *  @brief Implements a merge operation with an overlap count
 *  With a given wrap_count, the result will only contains shapes where
 *  the original shapes overlap at least "wrap_count" times.
 */
class DB_PUBLIC SelfOverlapMergeLocalOperation
  : public local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>
{
public:
  SelfOverlapMergeLocalOperation (unsigned int wrap_count);

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &result, const db::LocalProcessorBase *proc) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  unsigned int m_wrap_count;
};

/**
 *  @brief Converts polygons to edges
 */
class DB_PUBLIC PolygonToEdgeLocalOperation
  : public local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>
{
public:
  PolygonToEdgeLocalOperation (db::PropertiesRepository *target_pr, const db::PropertiesRepository *source_pr);

  virtual db::Coord dist () const { return 1; }
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const;

  virtual void do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::EdgeWithProperties> > &results, const db::LocalProcessorBase *proc) const;

private:
  mutable db::PropertyMapper m_pm;
};

} // namespace db

#endif



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

#include "gsiDecl.h"
#include "gsiEnums.h"

#include "dbCompoundOperation.h"
#include "dbRegionUtils.h"
#include "dbEdgesUtils.h"
#include "dbRegionLocalOperations.h"
#include "dbShapeCollectionUtils.h"
#include "tlString.h"

namespace gsi
{

template <class P>
static void check_non_null (P *p, const char *arg)
{
  if (!p) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Argument %s must not be null")), arg));
  }
}

template <class P>
static void check_non_null (const std::vector<P *> &pp, const char *arg)
{
  for (typename std::vector<P *>::const_iterator p = pp.begin (); p != pp.end (); ++p) {
    if (!*p) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Arguments %s must not be null")), arg));
    }
  }
}

static db::CompoundRegionOperationNode *new_primary ()
{
  return new db::CompoundRegionOperationPrimaryNode ();
}

static db::CompoundRegionOperationNode *new_foreign ()
{
  return new db::CompoundRegionOperationForeignNode ();
}

static db::CompoundRegionOperationNode *new_secondary (db::Region *region)
{
  check_non_null (region, "region");
  return new db::CompoundRegionOperationSecondaryNode (region);
}

static db::CompoundRegionOperationNode *new_empty (db::CompoundRegionOperationNode::ResultType type)
{
  return new db::CompoundRegionOperationEmptyNode (type);
}

static db::CompoundRegionOperationNode *new_logical_boolean (db::CompoundRegionLogicalBoolOperationNode::LogicalOp op, bool invert, const std::vector<db::CompoundRegionOperationNode *> &inputs)
{
  check_non_null (inputs, "inputs");
  return new db::CompoundRegionLogicalBoolOperationNode (op, invert, inputs);
}

static db::CompoundRegionOperationNode *new_geometrical_boolean (db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b)
{
  check_non_null (a, "a");
  check_non_null (b, "b");
  //  TODO: is this correct?
  if ((a->result_type () != db::CompoundRegionOperationNode::Region && a->result_type () != db::CompoundRegionOperationNode::Edges) ||
      (b->result_type () != db::CompoundRegionOperationNode::Region && b->result_type () != db::CompoundRegionOperationNode::Edges)) {
    throw tl::Exception ("Inputs for geometrical booleans must be either of Region or Edges type");
  }
  return new db::CompoundRegionGeometricalBoolOperationNode (op, a, b);
}

static db::CompoundRegionOperationNode *new_interacting (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse, size_t min_count, size_t max_count)
{
  check_non_null (a, "a");
  check_non_null (b, "b");
  //  TODO: is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, 0, true, inverse, min_count, max_count);
  } else if (b->result_type () == db::CompoundRegionOperationNode::Edges) {
    return new db::CompoundRegionInteractWithEdgeOperationNode (a, b, inverse, min_count, max_count);
  } else {
    throw tl::Exception ("Secondary input for interaction compound operation must be either of Region or Edges type");
  }
}

static db::CompoundRegionOperationNode *new_overlapping (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse, size_t min_count, size_t max_count)
{
  check_non_null (a, "a");
  check_non_null (b, "b");
  //  TODO: is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, 0, false, inverse, min_count, max_count);
  } else {
    throw tl::Exception ("Secondary input for overlapping compound operation must be of Region type");
  }
}

static db::CompoundRegionOperationNode *new_enclosing (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse, size_t min_count, size_t max_count)
{
  check_non_null (a, "a");
  check_non_null (b, "b");
  //  TODO: is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, -2, false, inverse, min_count, max_count);
  } else {
    throw tl::Exception ("Secondary input for enclosing compound operation must be of Region type");
  }
}

static db::CompoundRegionOperationNode *new_inside (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse)
{
  check_non_null (a, "a");
  check_non_null (b, "b");
  //  TODO: is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, -1, false, inverse);
  } else {
    throw tl::Exception ("Secondary input for inside compound operation must be of Region type");
  }
}

static db::CompoundRegionOperationNode *new_outside (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse)
{
  check_non_null (a, "a");
  check_non_null (b, "b");
  //  TODO: is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, 1, false, inverse);
  } else {
    throw tl::Exception ("Secondary input for outside compound operation must be of Region type");
  }
}

static db::CompoundRegionOperationNode *new_hulls (db::CompoundRegionOperationNode *input)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::HullExtractionProcessor (), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_holes (db::CompoundRegionOperationNode *input)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::HolesExtractionProcessor (), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_strange_polygons_filter (db::CompoundRegionOperationNode *input)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::StrangePolygonCheckProcessor (), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_smoothed (db::CompoundRegionOperationNode *input, db::Coord d, bool keep_hv)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::SmoothingProcessor (d, keep_hv), input, true /*processor is owned*/, d);
}

static db::CompoundRegionOperationNode *new_rounded_corners (db::CompoundRegionOperationNode *input, double rinner, double router, unsigned int n)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::RoundedCornersProcessor (rinner, router, n), input, true /*processor is owned*/, rinner /*dist adder*/);
}

static db::CompoundRegionOperationNode *new_case (const std::vector<db::CompoundRegionOperationNode *> &inputs)
{
  check_non_null (inputs, "inputs");
  return new db::CompoundRegionLogicalCaseSelectOperationNode (inputs);
}

static db::CompoundRegionOperationNode *new_join (const std::vector<db::CompoundRegionOperationNode *> &inputs)
{
  check_non_null (inputs, "inputs");
  return new db::CompoundRegionJoinOperationNode (inputs);
}

static db::CompoundRegionOperationNode *new_count_filter (db::CompoundRegionOperationNode *input, bool invert, size_t min_count, size_t max_count)
{
  return new db::CompoundRegionCountFilterNode (input, invert, min_count, max_count);
}

static db::CompoundRegionOperationNode *new_corners_as_rectangles (db::CompoundRegionOperationNode *input, double angle_start, bool include_angle_start, double angle_end, bool include_angle_end, db::Coord dim = 1)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::CornersAsRectangles (angle_start, include_angle_start, angle_end, include_angle_end, dim), input, true /*processor is owned*/, dim /*dist adder*/);
}

static db::CompoundRegionOperationNode *new_corners_as_dots (db::CompoundRegionOperationNode *input, double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
{
  check_non_null (input, "input");
  return new db::CompoundRegionToEdgeProcessingOperationNode (new db::CornersAsDots (angle_start, include_angle_start, angle_end, include_angle_end), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_corners_as_edge_pairs (db::CompoundRegionOperationNode *input, double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
{
  check_non_null (input, "input");
  return new db::CompoundRegionToEdgePairProcessingOperationNode (new db::CornersAsEdgePairs (angle_start, include_angle_start, angle_end, include_angle_end), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_extents (db::CompoundRegionOperationNode *input, db::Coord e)
{
  check_non_null (input, "input");
  if (input->result_type () == db::CompoundRegionOperationNode::EdgePairs) {
    return new db::CompoundRegionEdgePairToPolygonProcessingOperationNode (new db::extents_processor<db::EdgePair> (e, e), input, true /*processor is owned*/);
  } else if (input->result_type () == db::CompoundRegionOperationNode::EdgePairs) {
    return new db::CompoundRegionEdgeToPolygonProcessingOperationNode (new db::extents_processor<db::Edge> (e, e), input, true /*processor is owned*/);
  } else if (input->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionProcessingOperationNode (new db::extents_processor<db::Polygon> (e, e), input, true /*processor is owned*/);
  } else {
    input->keep ();
    return input;
  }
}

static db::CompoundRegionOperationNode *new_relative_extents (db::CompoundRegionOperationNode *input, double fx1, double fy1, double fx2, double fy2, db::Coord dx, db::Coord dy)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::RelativeExtents (fx1, fy1, fx2, fy2, dx, dy), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_relative_extents_as_edges (db::CompoundRegionOperationNode *input, double fx1, double fy1, double fx2, double fy2)
{
  check_non_null (input, "input");
  return new db::CompoundRegionToEdgeProcessingOperationNode (new db::RelativeExtentsAsEdges (fx1, fy1, fx2, fy2), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_convex_decomposition (db::CompoundRegionOperationNode *input, db::PreferredOrientation mode)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::ConvexDecomposition (mode), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_trapezoid_decomposition (db::CompoundRegionOperationNode *input, db::TrapezoidDecompositionMode mode)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::TrapezoidDecomposition (mode), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_polygon_breaker (db::CompoundRegionOperationNode *input, size_t max_vertex_count, double max_area_ratio)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::PolygonBreaker (max_vertex_count, max_area_ratio), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_sized (db::CompoundRegionOperationNode *input, db::Coord dx, db::Coord dy, unsigned int mode)
{
  check_non_null (input, "input");
  //  NOTE: the distance needs to be twice as we may want to see interactions between the post-size features and those interact when
  //  within twice the size range.
  db::Coord dist = 2 * std::max (db::Coord (0), std::max (dx, dy));
  return new db::CompoundRegionProcessingOperationNode (new db::PolygonSizer (dx, dy, mode), input, true /*processor is owned*/, dist);
}

static db::CompoundRegionOperationNode *new_merged (db::CompoundRegionOperationNode *input, bool min_coherence, unsigned int min_wc)
{
  check_non_null (input, "input");
  return new db::CompoundRegionMergeOperationNode (min_coherence, min_wc, input);
}

static db::CompoundRegionOperationNode *new_minkowski_sum_node1 (db::CompoundRegionOperationNode *input, const db::Edge &e)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::minkowski_sum_computation<db::Edge> (e), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowski_sum_node2 (db::CompoundRegionOperationNode *input, const db::Polygon &p)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::minkowski_sum_computation<db::Polygon> (p), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowski_sum_node3 (db::CompoundRegionOperationNode *input, const db::Box &p)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::minkowski_sum_computation<db::Box> (p), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowski_sum_node4 (db::CompoundRegionOperationNode *input, const std::vector<db::Point> &p)
{
  check_non_null (input, "input");
  return new db::CompoundRegionProcessingOperationNode (new db::minkowski_sum_computation<std::vector<db::Point> > (p), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_edges (db::CompoundRegionOperationNode *input)
{
  check_non_null (input, "input");
  if (input->result_type () == db::CompoundRegionOperationNode::EdgePairs) {
    return new db::CompoundRegionEdgePairToEdgeProcessingOperationNode (new db::EdgePairToEdgesProcessor (), input, true /*processor is owned*/);
  } else if (input->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), input, true /*processor is owned*/);
  } else {
    input->keep ();
    return input;
  }
}

static db::CompoundRegionOperationNode *new_edge_length_filter (db::CompoundRegionOperationNode *input, bool inverse, db::Edge::distance_type lmin, db::Edge::distance_type lmax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeFilterOperationNode (new db::EdgeLengthFilter (lmin, lmax, inverse), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_edge_length_sum_filter (db::CompoundRegionOperationNode *input, bool inverse, db::Edge::distance_type lmin, db::Edge::distance_type lmax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeFilterOperationNode (new db::EdgeLengthFilter (lmin, lmax, inverse), input, true /*processor is owned*/, true /*sum*/);
}

static db::CompoundRegionOperationNode *new_edge_orientation_filter (db::CompoundRegionOperationNode *input, bool inverse, double amin, bool include_amin, double amax, bool include_amax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeFilterOperationNode (new db::EdgeOrientationFilter (amin, include_amin, amax, include_amax, inverse), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_polygons (db::CompoundRegionOperationNode *input, db::Coord e)
{
  check_non_null (input, "input");
  if (input->result_type () == db::CompoundRegionOperationNode::EdgePairs) {
    return new db::CompoundRegionEdgePairToPolygonProcessingOperationNode (new db::EdgePairToPolygonProcessor (e), input, true /*processor is owned*/);
  } else if (input->result_type () == db::CompoundRegionOperationNode::Edges) {
    return new db::CompoundRegionEdgeToPolygonProcessingOperationNode (new db::ExtendedEdgeProcessor (e), input, true /*processor is owned*/);
  } else {
    input->keep ();
    return input;
  }
}

static db::CompoundRegionOperationNode *new_extended (db::CompoundRegionOperationNode *input, db::Coord ext_b, db::Coord ext_e, db::Coord ext_o, db::Coord ext_i)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeToPolygonProcessingOperationNode (new db::ExtendedEdgeProcessor (ext_b, ext_e, ext_o, ext_i), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_extended_in (db::CompoundRegionOperationNode *input, db::Coord e)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeToPolygonProcessingOperationNode (new db::ExtendedEdgeProcessor (0, 0, 0, e), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_extended_out (db::CompoundRegionOperationNode *input, db::Coord e)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeToPolygonProcessingOperationNode (new db::ExtendedEdgeProcessor (0, 0, e, 0), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_edge_pair_to_first_edges (db::CompoundRegionOperationNode *input)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgePairToEdgeProcessingOperationNode (new db::EdgePairToFirstEdgesProcessor (), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_edge_pair_to_second_edges (db::CompoundRegionOperationNode *input)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgePairToEdgeProcessingOperationNode (new db::EdgePairToSecondEdgesProcessor (), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_check_node (db::CompoundRegionOperationNode *other, db::edge_relation_type rel, bool different_polygons, db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  check_non_null (other, "other");
  return new db::CompoundRegionCheckOperationNode (0, other, rel, different_polygons, d,
    db::RegionCheckOptions (whole_edges,
                            metrics,
                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                            shielded,
                            opposite_filter,
                            rect_filter,
                            negative)
  );
}

static db::CompoundRegionOperationNode *new_width_check (db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, bool negative)
{
  db::RegionCheckOptions options (whole_edges,
                                  metrics,
                                  ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                  min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                  max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                  shielded);
  options.negative = negative;
  return new db::CompoundRegionToEdgePairProcessingOperationNode (new db::SinglePolygonCheck (db::WidthRelation, d, options), new_primary (), true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_space_or_isolated_check (db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative, bool isolated)
{
  //  NOTE: we have to use the "foreign" scheme with a filter because only this scheme
  //  guarantees that all subject shapes are visited and receive all intruders. Having all intruders is crucial for the
  //  semantics of the "drc" feature
  return new_check_node (new_foreign (), db::SpaceRelation, isolated, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative);
}

static db::CompoundRegionOperationNode *new_space_check (db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  return new_space_or_isolated_check (d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative, false);
}

static db::CompoundRegionOperationNode *new_isolated_check (db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  return new_space_or_isolated_check (d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative, true);
}

static db::CompoundRegionOperationNode *new_notch_check (db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, bool negative)
{
  db::RegionCheckOptions options (whole_edges,
                                  metrics,
                                  ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                  min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                                  max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> (),
                                  shielded);
  options.negative = negative;
  return new db::CompoundRegionToEdgePairProcessingOperationNode (new db::SinglePolygonCheck (db::SpaceRelation, d, options), new_primary (), true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_separation_check (db::CompoundRegionOperationNode *other, db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  return new_check_node (other, db::SpaceRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative);
}

static db::CompoundRegionOperationNode *new_overlap_check (db::CompoundRegionOperationNode *other, db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  return new_check_node (other, db::WidthRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative);
}

static db::CompoundRegionOperationNode *new_enclosing_check (db::CompoundRegionOperationNode *other, db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  return new_check_node (other, db::OverlapRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative);
}

static db::CompoundRegionOperationNode *new_enclosed_check (db::CompoundRegionOperationNode *other, db::Coord d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter, bool negative)
{
  return new_check_node (other, db::InsideRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded, opposite_filter, rect_filter, negative);
}

static db::CompoundRegionOperationNode *new_perimeter_filter (db::CompoundRegionOperationNode *input, bool inverse, db::coord_traits<db::Coord>::perimeter_type pmin, db::coord_traits<db::Coord>::perimeter_type pmax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RegionPerimeterFilter (pmin, pmax, inverse), input, true);
}

static db::CompoundRegionOperationNode *new_perimeter_sum_filter (db::CompoundRegionOperationNode *input, bool inverse, db::coord_traits<db::Coord>::perimeter_type pmin, db::coord_traits<db::Coord>::perimeter_type pmax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RegionPerimeterFilter (pmin, pmax, inverse), input, true, true /*sum of set*/);
}

static db::CompoundRegionOperationNode *new_hole_count_filter (db::CompoundRegionOperationNode *input, bool inverse, size_t hmin, size_t hmax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::HoleCountFilter (hmin, hmax, inverse), input, true);
}

static db::CompoundRegionOperationNode *new_area_filter (db::CompoundRegionOperationNode *input, bool inverse, db::coord_traits<db::Coord>::area_type amin, db::coord_traits<db::Coord>::area_type amax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (amin, amax, inverse), input, true);
}

static db::CompoundRegionOperationNode *new_area_sum_filter (db::CompoundRegionOperationNode *input, bool inverse, db::coord_traits<db::Coord>::area_type amin, db::coord_traits<db::Coord>::area_type amax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (amin, amax, inverse), input, true, true /*sum of set*/);
}

static db::CompoundRegionOperationNode *new_rectilinear_filter (db::CompoundRegionOperationNode *input, bool inverse)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RectilinearFilter (inverse), input, true);
}

static db::CompoundRegionOperationNode *new_rectangle_filter (db::CompoundRegionOperationNode *input, bool square, bool inverse)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RectangleFilter (square, inverse), input, true);
}

static db::CompoundRegionOperationNode *new_bbox_filter (db::CompoundRegionOperationNode *input, db::RegionBBoxFilter::parameter_type parameter, bool inverse, db::coord_traits<db::Coord>::distance_type vmin, db::coord_traits<db::Coord>::distance_type vmax)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RegionBBoxFilter (vmin, vmax, inverse, parameter), input, true);
}

static db::CompoundRegionOperationNode *new_ratio_filter (db::CompoundRegionOperationNode *input, db::RegionRatioFilter::parameter_type parameter, bool inverse, double vmin, bool vmin_included, double vmax, bool vmax_included)
{
  check_non_null (input, "input");
  return new db::CompoundRegionFilterOperationNode (new db::RegionRatioFilter (vmin, vmin_included, vmax, vmax_included, inverse, parameter), input, true);
}

static db::CompoundRegionOperationNode *new_start_segments (db::CompoundRegionOperationNode *input, db::Edges::length_type length, double fraction)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeProcessingOperationNode (new db::EdgeSegmentSelector (-1, length, fraction), input, true);
}

static db::CompoundRegionOperationNode *new_end_segments (db::CompoundRegionOperationNode *input, db::Edges::length_type length, double fraction)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeProcessingOperationNode (new db::EdgeSegmentSelector (1, length, fraction), input, true);
}

static db::CompoundRegionOperationNode *new_centers (db::CompoundRegionOperationNode *input, db::Edges::length_type length, double fraction)
{
  check_non_null (input, "input");
  return new db::CompoundRegionEdgeProcessingOperationNode (new db::EdgeSegmentSelector (0, length, fraction), input, true);
}

Class<db::CompoundRegionOperationNode> decl_CompoundRegionOperationNode ("db", "CompoundRegionOperationNode",
  gsi::constructor ("new_primary", &new_primary,
    "@brief Creates a node object representing the primary input"
  ) +
  gsi::constructor ("new_foreign", &new_foreign,
    "@brief Creates a node object representing the primary input without the current polygon"
  ) +
  gsi::constructor ("new_secondary", &new_secondary, gsi::arg ("region"),
    "@brief Creates a node object representing the secondary input from the given region"
  ) +
  gsi::constructor ("new_logical_boolean", &new_logical_boolean, gsi::arg ("op"), gsi::arg ("invert"), gsi::arg ("inputs"),
    "@brief Creates a node representing a logical boolean operation between the inputs.\n"
    "\n"
    "A logical AND operation will evaluate the arguments and render the subject shape when all arguments are non-empty. "
    "The logical OR operation will evaluate the arguments and render the subject shape when one argument is non-empty. "
    "Setting 'inverse' to true will reverse the result and return the subject shape when one argument is empty in the AND case and "
    "when all arguments are empty in the OR case."
  ) +
  gsi::constructor ("new_geometrical_boolean", &new_geometrical_boolean, gsi::arg ("op"), gsi::arg ("a"), gsi::arg ("b"),
    "@brief Creates a node representing a geometrical boolean operation between the inputs.\n"
  ) +
  gsi::constructor ("new_interacting", &new_interacting, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false), gsi::arg ("min_count", size_t (0)), gsi::arg ("max_count", std::numeric_limits<size_t>::max (), "unlimited"),
    "@brief Creates a node representing an interacting selection operation between the inputs.\n"
  ) +
  gsi::constructor ("new_overlapping", &new_overlapping, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false), gsi::arg ("min_count", size_t (0)), gsi::arg ("max_count", std::numeric_limits<size_t>::max (), "unlimited"),
    "@brief Creates a node representing an overlapping selection operation between the inputs.\n"
  ) +
  gsi::constructor ("new_enclosing", &new_enclosing, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false), gsi::arg ("min_count", size_t (0)), gsi::arg ("max_count", std::numeric_limits<size_t>::max (), "unlimited"),
    "@brief Creates a node representing an inside selection operation between the inputs.\n"
  ) +
  gsi::constructor ("new_inside", &new_inside, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false),
    "@brief Creates a node representing an inside selection operation between the inputs.\n"
  ) +
  gsi::constructor ("new_outside", &new_outside, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false),
    "@brief Creates a node representing an outside selection operation between the inputs.\n"
  ) +
  gsi::constructor ("new_hulls", &new_hulls, gsi::arg ("input"),
    "@brief Creates a node extracting the hulls from polygons.\n"
  ) +
  gsi::constructor ("new_holes", &new_holes, gsi::arg ("input"),
    "@brief Creates a node extracting the holes from polygons.\n"
  ) +
  gsi::constructor ("new_strange_polygons_filter", &new_strange_polygons_filter, gsi::arg ("input"),
    "@brief Creates a node extracting strange polygons.\n"
    "'strange polygons' are ones which cannot be oriented - e.g. '8' shape polygons."
  ) +
  gsi::constructor ("new_smoothed", &new_smoothed, gsi::arg ("input"), gsi::arg ("d"), gsi::arg ("keep_hv", false),
    "@brief Creates a node smoothing the polygons.\n"
    "@param d The tolerance to be applied for the smoothing.\n"
    "@param keep_hv If true, horizontal and vertical edges are maintained.\n"
  ) +
  gsi::constructor ("new_rounded_corners", &new_rounded_corners, gsi::arg ("input"), gsi::arg ("rinner"), gsi::arg ("router"), gsi::arg ("n"),
    "@brief Creates a node generating rounded corners.\n"
    "@param rinner The inner corner radius."
    "@param router The outer corner radius."
    "@param n The number if points per full circle."
  ) +
  gsi::constructor ("new_join", &new_join, gsi::arg ("inputs"),
    "@brief Creates a node that joins the inputs.\n"
  ) +
  gsi::constructor ("new_case", &new_case, gsi::arg ("inputs"),
    "@brief Creates a 'switch ladder' (case statement) compound operation node.\n"
    "\n"
    "The inputs are treated as a sequence of condition/result pairs: c1,r1,c2,r2 etc. If there is an odd number of inputs, the last "
    "element is taken as the default result. The implementation will evaluate c1 and if not empty, will render r1. Otherwise, c2 will be evaluated and r2 "
    "rendered if c2 isn't empty etc. If none of the conditions renders a non-empty set and a default result is present, the default will be "
    "returned. Otherwise, the result is empty."
  ) +
  gsi::constructor ("new_count_filter", &new_count_filter, gsi::arg ("inputs"), gsi::arg ("invert", false), gsi::arg ("min_count", size_t (0)), gsi::arg ("max_count", std::numeric_limits<size_t>::max ()),
    "@brief Creates a node selecting results but their shape count.\n"
  ) +
  gsi::constructor ("new_corners_as_rectangles", &new_corners_as_rectangles, gsi::arg ("input"), gsi::arg ("angle_min"), gsi::arg ("include_angle_min"), gsi::arg ("angle_max"), gsi::arg ("include_angle_max"), gsi::arg ("dim"),
    "@brief Creates a node turning corners into rectangles.\n"
  ) +
  gsi::constructor ("new_corners_as_dots", &new_corners_as_dots, gsi::arg ("input"), gsi::arg ("angle_min"), gsi::arg ("include_angle_min"), gsi::arg ("angle_max"), gsi::arg ("include_angle_max"),
    "@brief Creates a node turning corners into dots (single-point edges).\n"
  ) +
  gsi::constructor ("new_corners_as_edge_pairs", &new_corners_as_edge_pairs, gsi::arg ("input"), gsi::arg ("angle_min"), gsi::arg ("include_angle_min"), gsi::arg ("angle_max"), gsi::arg ("include_angle_max"),
    "@brief Creates a node turning corners into edge pairs containing the two edges adjacent to the corner.\n"
    "The first edge will be the incoming edge and the second one the outgoing edge.\n"
    "\n"
    "This feature has been introduced in version 0.27.1.\n"
  ) +
  gsi::constructor ("new_extents", &new_extents, gsi::arg ("input"), gsi::arg ("e", 0),
    "@brief Creates a node returning the extents of the objects.\n"
    "The 'e' parameter provides a generic enlargement which is applied to the boxes. This is helpful to cover dot-like edges or edge pairs in the input."
  ) +
  gsi::constructor ("new_relative_extents", &new_relative_extents, gsi::arg ("input"), gsi::arg ("fx1"), gsi::arg ("fy1"), gsi::arg ("fx2"), gsi::arg ("fy2"), gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Creates a node returning markers at specified locations of the extent (e.g. at the center).\n"
  ) +
  gsi::constructor ("new_relative_extents_as_edges", &new_relative_extents_as_edges, gsi::arg ("input"), gsi::arg ("fx1"), gsi::arg ("fy1"), gsi::arg ("fx2"), gsi::arg ("fy2"),
    "@brief Creates a node returning edges at specified locations of the extent (e.g. at the center).\n"
  ) +
  gsi::constructor ("new_convex_decomposition", &new_convex_decomposition, gsi::arg ("input"), gsi::arg ("mode"),
    "@brief Creates a node providing a composition into convex pieces.\n"
  ) +
  gsi::constructor ("new_trapezoid_decomposition", &new_trapezoid_decomposition, gsi::arg ("input"), gsi::arg ("mode"),
    "@brief Creates a node providing a composition into trapezoids.\n"
  ) +
  gsi::constructor ("new_polygon_breaker", &new_polygon_breaker, gsi::arg ("input"), gsi::arg ("max_vertex_count"), gsi::arg ("max_area_ratio"),
    "@brief Creates a node providing a composition into parts with less than the given number of points and a smaller area ratio.\n"
  ) +
  gsi::constructor ("new_sized", &new_sized, gsi::arg ("input"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Creates a node providing sizing.\n"
  ) +
  gsi::constructor ("new_merged", &new_merged, gsi::arg ("input"), gsi::arg ("min_coherence", false), gsi::arg ("min_wc", 0),
    "@brief Creates a node providing merged input polygons.\n"
  ) +
  gsi::constructor ("new_minkowski_sum|#new_minkowsky_sum", &new_minkowski_sum_node1, gsi::arg ("input"), gsi::arg ("e"),
    "@brief Creates a node providing a Minkowski sum with an edge.\n"
  ) +
  gsi::constructor ("new_minkowski_sum|#new_minkowsky_sum", &new_minkowski_sum_node2, gsi::arg ("input"), gsi::arg ("p"),
    "@brief Creates a node providing a Minkowski sum with a polygon.\n"
  ) +
  gsi::constructor ("new_minkowski_sum|#new_minkowsky_sum", &new_minkowski_sum_node3, gsi::arg ("input"), gsi::arg ("p"),
    "@brief Creates a node providing a Minkowski sum with a box.\n"
  ) +
  gsi::constructor ("new_minkowski_sum|#new_minkowsky_sum", &new_minkowski_sum_node4, gsi::arg ("input"), gsi::arg ("p"),
    "@brief Creates a node providing a Minkowski sum with a point sequence forming a contour.\n"
  ) +
  gsi::constructor ("new_width_check", &new_width_check, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("negative", false),
    "@brief Creates a node providing a width check.\n"
  ) +
  gsi::constructor ("new_space_check", &new_space_check, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false),
    "@brief Creates a node providing a space check.\n"
  ) +
  gsi::constructor ("new_isolated_check", &new_isolated_check, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false),
    "@brief Creates a node providing a isolated polygons (space between different polygons) check.\n"
  ) +
  gsi::constructor ("new_notch_check", &new_notch_check, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("negative", false),
    "@brief Creates a node providing a intra-polygon space check.\n"
  ) +
  gsi::constructor ("new_separation_check", &new_separation_check, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false),
    "@brief Creates a node providing a separation check.\n"
  ) +
  gsi::constructor ("new_overlap_check", &new_overlap_check, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false),
    "@brief Creates a node providing an overlap check.\n"
  ) +
  gsi::constructor ("new_enclosing_check", &new_enclosing_check, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false),
    "@brief Creates a node providing an inside (enclosure) check.\n"
  ) +
  gsi::constructor ("new_enclosed_check", &new_enclosed_check, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max."), gsi::arg ("shielded", true), gsi::arg ("opposite_filter", db::NoOppositeFilter, "NoOppositeFilter"), gsi::arg ("rect_filter", db::NoRectFilter, "NoRectFilter"), gsi::arg ("negative", false),
    "@brief Creates a node providing an enclosed (secondary enclosing primary) check.\n"
    "\n"
    "This method has been added in version 0.27.5.\n"
  ) +
  gsi::constructor ("new_perimeter_filter", &new_perimeter_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("pmin", 0), gsi::arg ("pmax", std::numeric_limits<db::coord_traits<db::Coord>::perimeter_type>::max (), "max"),
    "@brief Creates a node filtering the input by perimeter.\n"
    "This node renders the input if the perimeter is between pmin and pmax (exclusively). If 'inverse' is set to true, the "
    "input shape is returned if the perimeter is less than pmin (exclusively) or larger than pmax (inclusively)."
  ) +
  gsi::constructor ("new_perimeter_sum_filter", &new_perimeter_sum_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("amin", 0), gsi::arg ("amax", std::numeric_limits<db::coord_traits<db::Coord>::area_type>::max (), "max"),
    "@brief Creates a node filtering the input by area sum.\n"
    "Like \\new_perimeter_filter, but applies to the sum of all shapes in the current set.\n"
  ) +
  gsi::constructor ("new_area_filter", &new_area_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("amin", 0), gsi::arg ("amax", std::numeric_limits<db::coord_traits<db::Coord>::area_type>::max (), "max"),
    "@brief Creates a node filtering the input by area.\n"
    "This node renders the input if the area is between amin and amax (exclusively). If 'inverse' is set to true, the "
    "input shape is returned if the area is less than amin (exclusively) or larger than amax (inclusively)."
  ) +
  gsi::constructor ("new_area_sum_filter", &new_area_sum_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("amin", 0), gsi::arg ("amax", std::numeric_limits<db::coord_traits<db::Coord>::area_type>::max (), "max"),
    "@brief Creates a node filtering the input by area sum.\n"
    "Like \\new_area_filter, but applies to the sum of all shapes in the current set.\n"
  ) +
  gsi::constructor ("new_hole_count_filter", &new_hole_count_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("hmin", 0), gsi::arg ("hmax", std::numeric_limits<size_t>::max (), "max"),
    "@brief Creates a node filtering the input by number of holes per polygon.\n"
    "This node renders the input if the hole count is between hmin and hmax (exclusively). If 'inverse' is set to true, the "
    "input shape is returned if the hole count is less than hmin (exclusively) or larger than hmax (inclusively)."
  ) +
  gsi::constructor ("new_bbox_filter", &new_bbox_filter, gsi::arg ("input"), gsi::arg ("parameter"), gsi::arg ("inverse", false), gsi::arg ("pmin", 0), gsi::arg ("pmax", std::numeric_limits<db::coord_traits<db::Coord>::area_type>::max (), "max"),
    "@brief Creates a node filtering the input by bounding box parameters.\n"
    "This node renders the input if the specified bounding box parameter of the input shape is between pmin and pmax (exclusively). If 'inverse' is set to true, the "
    "input shape is returned if the parameter is less than pmin (exclusively) or larger than pmax (inclusively)."
  ) +
  gsi::constructor ("new_ratio_filter", &new_ratio_filter, gsi::arg ("input"), gsi::arg ("parameter"), gsi::arg ("inverse", false), gsi::arg ("pmin", 0.0), gsi::arg ("pmin_included", true), gsi::arg ("pmax", std::numeric_limits<double>::max (), "max"), gsi::arg ("pmax_included", true),
    "@brief Creates a node filtering the input by ratio parameters.\n"
    "This node renders the input if the specified ratio parameter of the input shape is between pmin and pmax. If 'pmin_included' is true, the range will include pmin. Same for 'pmax_included' and pmax. "
    "If 'inverse' is set to true, the input shape is returned if the parameter is not within the specified range."
  ) +
  gsi::constructor ("new_rectilinear_filter", &new_rectilinear_filter, gsi::arg ("input"), gsi::arg ("inverse", false),
    "@brief Creates a node filtering the input for rectilinear shapes (or non-rectilinear ones with 'inverse' set to 'true').\n"
  ) +
  gsi::constructor ("new_rectangle_filter", &new_rectangle_filter, gsi::arg ("input"), gsi::arg ("is_square", false), gsi::arg ("inverse", false),
    "@brief Creates a node filtering the input for rectangular or square shapes.\n"
    "If 'is_square' is true, only squares will be selected. If 'inverse' is true, the non-rectangle/non-square shapes are returned.\n"
  ) +
  gsi::constructor ("new_edges", &new_edges, gsi::arg ("input"),
    "@brief Creates a node converting polygons into its edges.\n"
  ) +
  gsi::constructor ("new_edge_length_filter", &new_edge_length_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("lmin", 0), gsi::arg ("lmax", std::numeric_limits<db::Edge::distance_type>::max (), "max"),
    "@brief Creates a node filtering edges by their length.\n"
  ) +
  gsi::constructor ("new_edge_length_sum_filter", &new_edge_length_sum_filter, gsi::arg ("input"), gsi::arg ("inverse", false), gsi::arg ("lmin", 0), gsi::arg ("lmax", std::numeric_limits<db::Edge::distance_type>::max (), "max"),
    "@brief Creates a node filtering edges by their length sum (over the local set).\n"
  ) +
  gsi::constructor ("new_edge_orientation_filter", &new_edge_orientation_filter, gsi::arg ("input"), gsi::arg ("inverse"), gsi::arg ("amin"), gsi::arg ("include_amin"), gsi::arg ("amax"), gsi::arg ("include_amax"),
    "@brief Creates a node filtering edges by their orientation.\n"
  ) +
  gsi::constructor ("new_polygons", &new_polygons, gsi::arg ("input"), gsi::arg ("e", 0),
    "@brief Creates a node converting the input to polygons.\n"
    "@param e The enlargement parameter when converting edges or edge pairs to polygons.\n"
  ) +
  gsi::constructor ("new_edge_pair_to_first_edges", &new_edge_pair_to_first_edges, gsi::arg ("input"),
    "@brief Creates a node delivering the first edge of each edges pair.\n"
  ) +
  gsi::constructor ("new_edge_pair_to_second_edges", &new_edge_pair_to_second_edges, gsi::arg ("input"),
    "@brief Creates a node delivering the second edge of each edges pair.\n"
  ) +
  gsi::constructor ("new_start_segments", &new_start_segments, gsi::arg ("input"), gsi::arg ("length"), gsi::arg ("fraction"),
    "@brief Creates a node delivering a part at the beginning of each input edge.\n"
  ) +
  gsi::constructor ("new_end_segments", &new_end_segments, gsi::arg ("input"), gsi::arg ("length"), gsi::arg ("fraction"),
    "@brief Creates a node delivering a part at the end of each input edge.\n"
  ) +
  gsi::constructor ("new_centers", &new_centers, gsi::arg ("input"), gsi::arg ("length"), gsi::arg ("fraction"),
    "@brief Creates a node delivering a part at the center of each input edge.\n"
  ) +
  gsi::constructor ("new_extended", &new_extended, gsi::arg ("input"), gsi::arg ("ext_b"), gsi::arg ("ext_e"), gsi::arg ("ext_o"), gsi::arg ("ext_i"),
    "@brief Creates a node delivering a polygonized version of the edges with the four extension parameters.\n"
  ) +
  gsi::constructor ("new_extended_in", &new_extended_in, gsi::arg ("input"), gsi::arg ("e"),
    "@brief Creates a node delivering a polygonized, inside-extended version of the edges.\n"
  ) +
  gsi::constructor ("new_extended_out", &new_extended_out, gsi::arg ("input"), gsi::arg ("e"),
    "@brief Creates a node delivering a polygonized, inside-extended version of the edges.\n"
  ) +
  gsi::constructor ("new_empty", &new_empty, gsi::arg ("type"),
    "@brief Creates a node delivering an empty result of the given type\n"
  ) +
  method ("distance=", &db::CompoundRegionOperationNode::set_dist, gsi::arg ("d"),
    "@brief Sets the distance value for this node"
    "Usually it's not required to provide a distance because the nodes compute a distance based on their "
    "operation. If necessary you can supply a distance. The processor will use this distance or the computed one, "
    "whichever is larger."
  ) +
  method ("distance", &db::CompoundRegionOperationNode::dist,
    "@brief Gets the distance value for this node"
  ) +
  method ("description=", &db::CompoundRegionOperationNode::set_description, gsi::arg ("d"),
    "@brief Sets the description for this node"
  ) +
  method ("description", &db::CompoundRegionOperationNode::description,
    "@brief Gets the description for this node"
  ) +
  method ("result_type", &db::CompoundRegionOperationNode::result_type,
    "@brief Gets the result type of this node"
  ),
  "@brief A base class for compound DRC operations\n"
  "\n"
  "This class is not intended to be used directly but rather provide a factory for various incarnations of "
  "compound operation nodes. Compound operations are a way to specify complex DRC operations put together "
  "by building a tree of operations. This operation tree then is executed with \\Region#complex_op and will act on "
  "individual clusters of shapes and their interacting neighbors.\n"
  "\n"
  "A basic concept to the compound operations is the 'subject' (primary) and 'intruder' (secondary) input. "
  "The 'subject' is the Region, 'complex_op' with the operation tree is executed on. 'intruders' are regions inserted into "
  "the equation through secondary input nodes created with \\new_secondary_node. The algorithm will execute the "
  "operation tree for every subject shape considering intruder shapes from the secondary inputs. The algorithm will "
  "only act on subject shapes primarily. As a consequence, 'lonely' intruder shapes without a subject shape are "
  "not considered at all. Only subject shapes trigger evaluation of the operation tree.\n"
  "\n"
  "The search distance for intruder shapes is determined by the operation and computed from the operation's requirements.\n"
  "\n"
  "NOTE: this feature is experimental and not deployed into the the DRC framework yet.\n"
  "\n"
  "This class has been introduced in version 0.27."
);

gsi::EnumIn<db::CompoundRegionOperationNode, db::CompoundRegionLogicalBoolOperationNode::LogicalOp> decl_dbCompoundRegionLogicalBoolOperationNode_LogicalOp ("db", "LogicalOp",
  gsi::enum_const ("LogAnd", db::CompoundRegionLogicalBoolOperationNode::LogicalOp::And,
    "@brief Indicates a logical '&&' (and)."
  ) +
  gsi::enum_const ("LogOr", db::CompoundRegionLogicalBoolOperationNode::LogicalOp::Or,
    "@brief Indicates a logical '||' (or)."
  ),
  "@brief This class represents the CompoundRegionOperationNode::LogicalOp enum\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

gsi::EnumIn<db::CompoundRegionOperationNode, db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp> decl_dbCompoundRegionGeometricalBoolOperationNode_GeometricalOp ("db", "GeometricalOp",
  gsi::enum_const ("And", db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp::And,
    "@brief Indicates a geometrical '&' (and)."
  ) +
  gsi::enum_const ("Not", db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp::Not,
    "@brief Indicates a geometrical '-' (not)."
  ) +
  gsi::enum_const ("Xor", db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp::Xor,
    "@brief Indicates a geometrical '^' (xor)."
  ) +
  gsi::enum_const ("Or", db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp::Or,
    "@brief Indicates a geometrical '|' (or)."
  ),
  "@brief This class represents the CompoundRegionOperationNode::GeometricalOp enum\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

gsi::EnumIn<db::CompoundRegionOperationNode, db::CompoundRegionOperationNode::ResultType> decl_dbCompoundRegionOperationNode_ResultType ("db", "ResultType",
  gsi::enum_const ("Region", db::CompoundRegionOperationNode::ResultType::Region,
    "@brief Indicates polygon result type."
  ) +
  gsi::enum_const ("Edges", db::CompoundRegionOperationNode::ResultType::Edges,
    "@brief Indicates edge result type."
  ) +
  gsi::enum_const ("EdgePairs", db::CompoundRegionOperationNode::ResultType::EdgePairs,
    "@brief Indicates edge pair result type."
  ),
  "@brief This class represents the CompoundRegionOperationNode::ResultType enum\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

gsi::Enum<db::TrapezoidDecompositionMode> decl_dbTrapezoidDecompositionMode ("db", "TrapezoidDecompositionMode",
  gsi::enum_const ("TD_simple", db::TrapezoidDecompositionMode::TD_simple,
    "@brief Indicates unspecific decomposition."
  ) +
  gsi::enum_const ("TD_htrapezoids", db::TrapezoidDecompositionMode::TD_htrapezoids,
    "@brief Indicates horizontal trapezoid decomposition."
  ) +
  gsi::enum_const ("TD_vtrapezoids", db::TrapezoidDecompositionMode::TD_vtrapezoids,
    "@brief Indicates vertical trapezoid decomposition."
  ),
  "@brief This class represents the TrapezoidDecompositionMode enum used within trapezoid decomposition\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

gsi::Enum<db::PreferredOrientation> decl_dbPreferredOrientation ("db", "PreferredOrientation",
  gsi::enum_const ("PO_any", db::PreferredOrientation::PO_any,
    "@brief Indicates any orientation."
  ) +
  gsi::enum_const ("PO_horizontal", db::PreferredOrientation::PO_horizontal,
    "@brief Indicates horizontal orientation."
  ) +
  gsi::enum_const ("PO_vertical", db::PreferredOrientation::PO_vertical,
    "@brief Indicates vertical orientation."
  ) +
  gsi::enum_const ("PO_htrapezoids", db::PreferredOrientation::PO_htrapezoids,
    "@brief Indicates horizontal trapezoid decomposition."
  ) +
  gsi::enum_const ("PO_vtrapezoids", db::PreferredOrientation::PO_vtrapezoids,
    "@brief Indicates vertical trapezoid decomposition."
  ),
  "@brief This class represents the PreferredOrientation enum used within polygon decomposition\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

gsi::EnumIn<db::CompoundRegionOperationNode, db::RegionBBoxFilter::parameter_type> decl_dbRegionBBoxFilter_ParameterType ("db", "ParameterType",
  gsi::enum_const ("BoxWidth", db::RegionBBoxFilter::BoxWidth,
    "@brief Measures the width of the bounding box\n"
  ) +
  gsi::enum_const ("BoxHeight", db::RegionBBoxFilter::BoxHeight,
    "@brief Measures the height of the bounding box\n"
  ) +
  gsi::enum_const ("BoxMaxDim", db::RegionBBoxFilter::BoxMaxDim,
    "@brief Measures the maximum dimension of the bounding box\n"
  ) +
  gsi::enum_const ("BoxMinDim", db::RegionBBoxFilter::BoxMinDim,
    "@brief Measures the minimum dimension of the bounding box\n"
  ) +
  gsi::enum_const ("BoxAverageDim", db::RegionBBoxFilter::BoxAverageDim,
    "@brief Measures the average of width and height of the bounding box\n"
  ),
  "@brief This class represents the parameter type enum used in \\CompoundRegionOperationNode#new_bbox_filter\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

gsi::EnumIn<db::CompoundRegionOperationNode, db::RegionRatioFilter::parameter_type> decl_dbRegionRatioFilter_ParameterType ("db", "RatioParameterType",
  gsi::enum_const ("AreaRatio", db::RegionRatioFilter::AreaRatio,
    "@brief Measures the area ratio (bounding box area / polygon area)\n"
  ) +
  gsi::enum_const ("AspectRatio", db::RegionRatioFilter::AspectRatio,
    "@brief Measures the aspect ratio of the bounding box (larger / smaller dimension)\n"
  ) +
  gsi::enum_const ("RelativeHeight", db::RegionRatioFilter::RelativeHeight,
    "@brief Measures the relative height (height / width)\n"
  ),
  "@brief This class represents the parameter type enum used in \\CompoundRegionOperationNode#new_ratio_filter\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

}

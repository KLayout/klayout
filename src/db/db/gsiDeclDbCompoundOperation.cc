
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

namespace gsi
{

static db::CompoundRegionOperationNode *new_primary ()
{
  return new db::CompoundRegionOperationPrimaryNode ();
}

static db::CompoundRegionOperationNode *new_secondary (db::Region *region)
{
  return new db::CompoundRegionOperationSecondaryNode (region);
}

static db::CompoundRegionOperationNode *new_logical_boolean (db::CompoundRegionLogicalBoolOperationNode::LogicalOp op, bool invert, const std::vector<db::CompoundRegionOperationNode *> &inputs)
{
  return new db::CompoundRegionLogicalBoolOperationNode (op, invert, inputs);
}

static db::CompoundRegionOperationNode *new_geometrical_boolean (db::CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b)
{
  // @@@ is this correct?
  if ((a->result_type () != db::CompoundRegionOperationNode::Region && a->result_type () != db::CompoundRegionOperationNode::Edges) ||
      (b->result_type () != db::CompoundRegionOperationNode::Region && b->result_type () != db::CompoundRegionOperationNode::Edges)) {
    throw tl::Exception ("Inputs for geometrical booleans must be either of Region or Edges type");
  }
  return new db::CompoundRegionGeometricalBoolOperationNode (op, a, b);
}

static db::CompoundRegionOperationNode *new_interacting (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse, size_t min_count, size_t max_count)
{
  // @@@ is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, 0, true, inverse, min_count, max_count);
  } else if (b->result_type () == db::CompoundRegionOperationNode::Edges) {
    return new db::CompoundRegionInteractWithEdgeOperationNode (a, b, 0, true, inverse, min_count, max_count);
  } else {
    throw tl::Exception ("Secondary input for interaction compound operation must be either of Region or Edges type");
  }
}

static db::CompoundRegionOperationNode *new_overlapping (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse, size_t min_count, size_t max_count)
{
  // @@@ is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, 0, false, inverse, min_count, max_count);
  } else {
    throw tl::Exception ("Secondary input for overlapping compound operation must be of Region type");
  }
}

static db::CompoundRegionOperationNode *new_inside (db::CompoundRegionOperationNode *a, db::CompoundRegionOperationNode *b, bool inverse)
{
  // @@@ is this correct?
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
  // @@@ is this correct?
  if (a->result_type () != db::CompoundRegionOperationNode::Region) {
    throw tl::Exception ("Primary input for interaction compound operation must be of Region type");
  }
  if (b->result_type () == db::CompoundRegionOperationNode::Region) {
    return new db::CompoundRegionInteractOperationNode (a, b, 1, false, inverse);
  } else {
    throw tl::Exception ("Secondary input for outside compound operation must be of Region type");
  }
}

static db::CompoundRegionOperationNode *new_case (const std::vector<db::CompoundRegionOperationNode *> &inputs)
{
  return new db::CompoundRegionLogicalCaseSelectOperationNode (false, inputs);
}

static db::CompoundRegionOperationNode *new_corners_as_rectangles_node (db::CompoundRegionOperationNode *input, double angle_start, double angle_end, db::Coord dim = 1)
{
  return new db::CompoundRegionProcessingOperationNode (new db::CornersAsRectangles (angle_start, angle_end, dim), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_corners_as_dots_node (db::CompoundRegionOperationNode *input, double angle_start, double angle_end)
{
  return new db::CompoundRegionToEdgeProcessingOperationNode (new db::CornersAsDots (angle_start, angle_end), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_relative_extents_node (db::CompoundRegionOperationNode *input, double fx1, double fy1, double fx2, double fy2, db::Coord dx, db::Coord dy)
{
  return new db::CompoundRegionProcessingOperationNode (new db::RelativeExtents (fx1, fy1, fx2, fy2, dx, dy), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_relative_extents_as_edges_node (db::CompoundRegionOperationNode *input, double fx1, double fy1, double fx2, double fy2)
{
  return new db::CompoundRegionToEdgeProcessingOperationNode (new db::RelativeExtentsAsEdges (fx1, fy1, fx2, fy2), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_convex_decomposition_node (db::CompoundRegionOperationNode *input, db::PreferredOrientation mode)
{
  return new db::CompoundRegionProcessingOperationNode (new db::ConvexDecomposition (mode), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_trapezoid_decomposition_node (db::CompoundRegionOperationNode *input, db::TrapezoidDecompositionMode mode)
{
  return new db::CompoundRegionProcessingOperationNode (new db::TrapezoidDecomposition (mode), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_polygon_breaker_node (db::CompoundRegionOperationNode *input, size_t max_vertex_count, double max_area_ratio)
{
  return new db::CompoundRegionProcessingOperationNode (new db::PolygonBreaker (max_vertex_count, max_area_ratio), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_size_node (db::CompoundRegionOperationNode *input, db::Coord dx, db::Coord dy, unsigned int mode)
{
  return new db::CompoundRegionProcessingOperationNode (new db::PolygonSizer (dx, dy, mode), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowsky_sum_node1 (db::CompoundRegionOperationNode *input, const db::Edge &e)
{
  return new db::CompoundRegionProcessingOperationNode (new db::minkowsky_sum_computation<db::Edge> (e), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowsky_sum_node2 (db::CompoundRegionOperationNode *input, const db::Polygon &p)
{
  return new db::CompoundRegionProcessingOperationNode (new db::minkowsky_sum_computation<db::Polygon> (p), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowsky_sum_node3 (db::CompoundRegionOperationNode *input, const db::Box &p)
{
  return new db::CompoundRegionProcessingOperationNode (new db::minkowsky_sum_computation<db::Box> (p), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_minkowsky_sum_node4 (db::CompoundRegionOperationNode *input, const std::vector<db::Point> &p)
{
  return new db::CompoundRegionProcessingOperationNode (new db::minkowsky_sum_computation<std::vector<db::Point> > (p), input, true /*processor is owned*/);
}

static db::CompoundRegionOperationNode *new_width_check_node (db::Coord d, bool whole_edges, db::metrics_type metrics, double ignore_angle, db::coord_traits<db::Coord>::distance_type min_projection, db::coord_traits<db::Coord>::distance_type max_projection, bool shielded)
{
  return new db::CompoundRegionCheckOperationNode (db::WidthRelation, false, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded);
}

static db::CompoundRegionOperationNode *new_space_check_node (db::Coord d, bool whole_edges, db::metrics_type metrics, double ignore_angle, db::coord_traits<db::Coord>::distance_type min_projection, db::coord_traits<db::Coord>::distance_type max_projection, bool shielded)
{
  return new db::CompoundRegionCheckOperationNode (db::SpaceRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded);
}

static db::CompoundRegionOperationNode *new_notch_check_node (db::Coord d, bool whole_edges, db::metrics_type metrics, double ignore_angle, db::coord_traits<db::Coord>::distance_type min_projection, db::coord_traits<db::Coord>::distance_type max_projection, bool shielded)
{
  return new db::CompoundRegionCheckOperationNode (db::SpaceRelation, false, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded);
}

static db::CompoundRegionOperationNode *new_separation_check_node (db::CompoundRegionOperationNode *input, db::Coord d, bool whole_edges, db::metrics_type metrics, double ignore_angle, db::coord_traits<db::Coord>::distance_type min_projection, db::coord_traits<db::Coord>::distance_type max_projection, bool shielded)
{
  return new db::CompoundRegionCheckOperationNode (input, db::SpaceRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded);
}

static db::CompoundRegionOperationNode *new_overlap_check_node (db::CompoundRegionOperationNode *input, db::Coord d, bool whole_edges, db::metrics_type metrics, double ignore_angle, db::coord_traits<db::Coord>::distance_type min_projection, db::coord_traits<db::Coord>::distance_type max_projection, bool shielded)
{
  return new db::CompoundRegionCheckOperationNode (input, db::OverlapRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded);
}

static db::CompoundRegionOperationNode *new_inside_check_node (db::CompoundRegionOperationNode *input, db::Coord d, bool whole_edges, db::metrics_type metrics, double ignore_angle, db::coord_traits<db::Coord>::distance_type min_projection, db::coord_traits<db::Coord>::distance_type max_projection, bool shielded)
{
  return new db::CompoundRegionCheckOperationNode (input, db::InsideRelation, true, d, whole_edges, metrics, ignore_angle, min_projection, max_projection, shielded);
}

Class<db::CompoundRegionOperationNode> decl_CompoundRegionOperationNode ("db", "CompoundRegionOperationNode",
  gsi::constructor ("new_primary", &new_primary,
    "@brief Creates a node object representing the primary input"
  ) +
  gsi::constructor ("new_secondary", &new_secondary, gsi::arg ("region"),
    "@brief Creates a node object representing the secondary input from the given region"
  ) +
  gsi::constructor ("new_logical_boolean", &new_logical_boolean, gsi::arg ("op"), gsi::arg ("invert"), gsi::arg ("inputs"),
    "@brief Creates a node representing a logical boolean operation between the inputs.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_geometrical_boolean", &new_geometrical_boolean, gsi::arg ("op"), gsi::arg ("a"), gsi::arg ("b"),
    "@brief Creates a node representing a geometrical boolean operation between the inputs.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_interacting", &new_interacting, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false), gsi::arg ("min_count", size_t (0)), gsi::arg ("max_count", std::numeric_limits<size_t>::max (), "unlimited"),
    "@brief Creates a node representing an interacting selection operation between the inputs.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_overlapping", &new_overlapping, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false), gsi::arg ("min_count", size_t (0)), gsi::arg ("max_count", std::numeric_limits<size_t>::max (), "unlimited"),
    "@brief Creates a node representing an overlapping selection operation between the inputs.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_inside", &new_inside, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false),
    "@brief Creates a node representing an inside selection operation between the inputs.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_outside", &new_outside, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("inverse", false),
    "@brief Creates a node representing an outside selection operation between the inputs.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_case", &new_case, gsi::arg ("inputs"),
    "@brief Creates a 'switch ladder' (case statement) compound operation node.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_corners_as_rectangles", &new_corners_as_rectangles_node, gsi::arg ("input"), gsi::arg ("angle_start"), gsi::arg ("angle_end"), gsi::arg ("dim"),
    "@brief Creates a node turning corners into rectangles.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_corners_as_dots", &new_corners_as_dots_node, gsi::arg ("input"), gsi::arg ("angle_start"), gsi::arg ("angle_end"),
    "@brief Creates a node turning corners into dots (single-point edges).\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_relative_extents", &new_relative_extents_node, gsi::arg ("input"), gsi::arg ("fx1"), gsi::arg ("fy1"), gsi::arg ("fx2"), gsi::arg ("fy2"), gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Creates a node returning markers at specified locations of the extend (e.g. at the center).\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_relative_extents_as_edges", &new_relative_extents_as_edges_node, gsi::arg ("input"), gsi::arg ("fx1"), gsi::arg ("fy1"), gsi::arg ("fx2"), gsi::arg ("fy2"),
    "@brief Creates a node returning edges at specified locations of the extend (e.g. at the center).\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_convex_decomposition", &new_convex_decomposition_node, gsi::arg ("input"), gsi::arg ("mode"),
    "@brief Creates a node providing a composition into convex pieces.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_trapezoid_decomposition", &new_trapezoid_decomposition_node, gsi::arg ("input"), gsi::arg ("mode"),
    "@brief Creates a node providing a composition into trapezoids.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_polygon_breaker_node", &new_polygon_breaker_node, gsi::arg ("input"), gsi::arg ("max_vertex_count"), gsi::arg ("max_area_ratio"),
    "@brief Creates a node providing a composition into parts with less than the given number of points and a smaller area ratio.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_size_node", &new_size_node, gsi::arg ("input"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Creates a node providing sizing.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_minkowsky_sum", &new_minkowsky_sum_node1, gsi::arg ("input"), gsi::arg ("e"),
    "@brief Creates a node providing a Minkowsky sum with an edge.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_minkowsky_sum", &new_minkowsky_sum_node2, gsi::arg ("input"), gsi::arg ("p"),
    "@brief Creates a node providing a Minkowsky sum with a polygon.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_minkowsky_sum", &new_minkowsky_sum_node3, gsi::arg ("input"), gsi::arg ("p"),
    "@brief Creates a node providing a Minkowsky sum with a box.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_minkowsky_sum", &new_minkowsky_sum_node4, gsi::arg ("input"), gsi::arg ("p"),
    "@brief Creates a node providing a Minkowsky sum with a point sequence forming a contour.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_width_check", &new_width_check_node, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian), gsi::arg ("ignore_angle", 90.0), gsi::arg ("min_projection", db::coord_traits<db::Coord>::distance_type (0)), gsi::arg ("max_projection", std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max (), "max."), gsi::arg ("shielded", true),
    "@brief Creates a node providing a width check.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_space_check", &new_space_check_node, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian), gsi::arg ("ignore_angle", 90.0), gsi::arg ("min_projection", db::coord_traits<db::Coord>::distance_type (0)), gsi::arg ("max_projection", std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max (), "max."), gsi::arg ("shielded", true),
    "@brief Creates a node providing a space check.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_notch_check", &new_notch_check_node, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian), gsi::arg ("ignore_angle", 90.0), gsi::arg ("min_projection", db::coord_traits<db::Coord>::distance_type (0)), gsi::arg ("max_projection", std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max (), "max."), gsi::arg ("shielded", true),
    "@brief Creates a node providing a space check.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_separation_check", &new_separation_check_node, gsi::arg ("input"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian), gsi::arg ("ignore_angle", 90.0), gsi::arg ("min_projection", db::coord_traits<db::Coord>::distance_type (0)), gsi::arg ("max_projection", std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max (), "max."), gsi::arg ("shielded", true),
    "@brief Creates a node providing a separation check.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_overlap_check", &new_overlap_check_node, gsi::arg ("input"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian), gsi::arg ("ignore_angle", 90.0), gsi::arg ("min_projection", db::coord_traits<db::Coord>::distance_type (0)), gsi::arg ("max_projection", std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max (), "max."), gsi::arg ("shielded", true),
    "@brief Creates a node providing an overlap check.\n"
    "\n"
    "@@@ TODO.\n"
  ) +
  gsi::constructor ("new_inside_check", &new_inside_check_node, gsi::arg ("input"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian), gsi::arg ("ignore_angle", 90.0), gsi::arg ("min_projection", db::coord_traits<db::Coord>::distance_type (0)), gsi::arg ("max_projection", std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max (), "max."), gsi::arg ("shielded", true),
    "@brief Creates a node providing an inside (enclosure) check.\n"
    "\n"
    "@@@ TODO.\n"
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
  "@brief A base class for compound operations\n"
  "\n"
  "This class has been introduced in version 0.27."
);

gsi::EnumIn<db::CompoundRegionOperationNode, db::CompoundRegionLogicalBoolOperationNode::LogicalOp> decl_dbCompoundRegionLogicalBoolOperationNode_LogicalOp ("db", "LogicalOp",
  gsi::enum_const ("And", db::CompoundRegionLogicalBoolOperationNode::LogicalOp::And,
    "@brief Indicates a logical '&&' (and)."
  ) +
  gsi::enum_const ("Or", db::CompoundRegionLogicalBoolOperationNode::LogicalOp::Or,
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

gsi::Enum<db::metrics_type> decl_dbMetricsType ("db", "MetricsType",
  gsi::enum_const ("Euclidian", db::metrics_type::Euclidian,
    "@brief Euclidian metrics\n"
    "\n"
    "The distance between two points is defined as the euclidian\n"
    "distance, i.e. d = sqrt(dx * dx + dy * dy).\n"
    "All points within a circle with radius r around another point \n"
    "have a distance less than r to this point.\n"
  ) +
  gsi::enum_const ("Square", db::metrics_type::Square,
    "@brief Square metrics\n"
    "\n"
    "The distance between two points is the minimum of x and\n"
    "y distance, i.e. d = min(abs(dx), abs(dy)).\n"
    "All points within a square with length 2*r round another point\n"
    "have a distance less than r to this point.\n"
  ) +
  gsi::enum_const ("Projection", db::metrics_type::Projection,
    "@brief Projection metrics\n"
    "\n"
    "The distance between a point and another point on an edge\n"
    "is measured by the distance of the point to the edge.\n"
  ),
  "@brief This class represents the MetricsType enum used within DRC functions\n"
  "\n"
  "This enum has been introduced in version 0.27."
);

}


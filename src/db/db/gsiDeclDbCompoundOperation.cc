
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

//  @@@ more ...
//   CompoundRegionProcessingOperationNode with various processors
//   CompoundRegionSizeOperationNode
//   CompoundRegionToEdgeProcessingOperationNode
//   CompoundRegionToEdgePairProcessingOperationNode
//   CompoundRegionCheckOperationNode


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

}


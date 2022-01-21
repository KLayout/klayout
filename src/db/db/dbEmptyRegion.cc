
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "dbEmptyRegion.h"
#include "dbEmptyEdges.h"
#include "dbEmptyEdgePairs.h"
#include "dbRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  EmptyRegion implementation

EmptyRegion::EmptyRegion ()
{
  //  .. nothing yet ..
}

EmptyRegion::~EmptyRegion ()
{
  //  .. nothing yet ..
}

EmptyRegion::EmptyRegion (const EmptyRegion &other)
  : RegionDelegate (other)
{
  // .. nothing yet ..
}

RegionDelegate *
EmptyRegion::clone () const
{
  return new EmptyRegion (*this);
}

RegionDelegate *
EmptyRegion::add_in_place (const Region &other)
{
  return add (other);
}

RegionDelegate *
EmptyRegion::add (const Region &other) const
{
  return other.delegate ()->clone ();
}

RegionDelegate *
EmptyRegion::xor_with (const Region &other) const
{
  return or_with (other);
}

RegionDelegate *
EmptyRegion::or_with (const Region &other) const
{
  if (other.empty ()) {
    return new EmptyRegion ();
  } else if (! other.strict_handling ()) {
    return other.delegate ()->clone ();
  } else {
    return other.delegate ()->merged ();
  }
}

EdgesDelegate *
EmptyRegion::processed_to_edges (const PolygonToEdgeProcessorBase &) const
{
  return new EmptyEdges ();
}

EdgePairsDelegate *
EmptyRegion::processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::cop_to_edge_pairs (db::CompoundRegionOperationNode &)
{
  return new EmptyEdgePairs ();
}

RegionDelegate *
EmptyRegion::cop_to_region (db::CompoundRegionOperationNode &)
{
  return new EmptyRegion ();
}

EdgesDelegate *
EmptyRegion::cop_to_edges (db::CompoundRegionOperationNode &)
{
  return new EmptyEdges ();
}

EdgePairsDelegate *
EmptyRegion::width_check (db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::space_check (db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::isolated_check (db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::notch_check (db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::enclosing_check (const Region &, db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::overlap_check (const Region &, db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::separation_check (const Region &, db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::inside_check (const Region &, db::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::grid_check (db::Coord, db::Coord) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::angle_check (double, double, bool) const
{
  return new EmptyEdgePairs ();
}

EdgesDelegate *
EmptyRegion::edges (const EdgeFilterBase *) const
{
  return new EmptyEdges ();
}

bool
EmptyRegion::equals (const Region &other) const
{
  return other.empty ();
}

bool
EmptyRegion::less (const Region &other) const
{
  return other.empty () ? false : true;
}

}


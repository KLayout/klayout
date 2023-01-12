
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


#include "dbEmptyEdges.h"
#include "dbEmptyEdgePairs.h"
#include "dbEmptyRegion.h"
#include "dbEdges.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  EmptyEdges implementation

EmptyEdges::EmptyEdges ()
{
  //  .. nothing yet ..
}

EmptyEdges::~EmptyEdges ()
{
  //  .. nothing yet ..
}

EmptyEdges::EmptyEdges (const EmptyEdges &other)
  : EdgesDelegate (other)
{
  // .. nothing yet ..
}

EdgesDelegate *
EmptyEdges::clone () const
{
  return new EmptyEdges (*this);
}

RegionDelegate *
EmptyEdges::extended (coord_type, coord_type, coord_type, coord_type, bool) const
{
  return new EmptyRegion ();
}

RegionDelegate *
EmptyEdges::pull_interacting (const Region &) const
{
  return new EmptyRegion ();
}

EdgePairsDelegate *
EmptyEdges::processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &) const
{
  return new EmptyEdgePairs ();
}

RegionDelegate *
EmptyEdges::processed_to_polygons (const EdgeToPolygonProcessorBase &) const
{
  return new EmptyRegion ();
}

EdgePairsDelegate *
EmptyEdges::width_check (db::Coord, const db::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::space_check (db::Coord, const db::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::enclosing_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::overlap_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::separation_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::inside_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgesDelegate *
EmptyEdges::add_in_place (const Edges &other)
{
  return add (other);
}

EdgesDelegate *
EmptyEdges::add (const Edges &other) const
{
  return other.delegate ()->clone ();
}

EdgesDelegate *
EmptyEdges::xor_with (const Edges &other) const
{
  return or_with (other);
}

EdgesDelegate *
EmptyEdges::or_with (const Edges &other) const
{
  if (other.empty ()) {
    return new EmptyEdges ();
  } else if (! other.strict_handling ()) {
    return other.delegate ()->clone ();
  } else {
    return other.delegate ()->merged ();
  }
}

bool
EmptyEdges::equals (const Edges &other) const
{
  return other.empty ();
}

bool
EmptyEdges::less (const Edges &other) const
{
  return other.empty () ? false : true;
}

}


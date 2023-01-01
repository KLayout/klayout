
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


#include "dbEmptyEdgePairs.h"
#include "dbEmptyRegion.h"
#include "dbEmptyEdges.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------

EmptyEdgePairs::EmptyEdgePairs ()
{
  //  .. nothing yet ..
}

EmptyEdgePairs::EmptyEdgePairs (const EmptyEdgePairs &other)
  : EdgePairsDelegate (other)
{
  // .. nothing yet ..
}

EdgePairsDelegate *
EmptyEdgePairs::clone () const
{
  return new EmptyEdgePairs (*this);
}

RegionDelegate *
EmptyEdgePairs::polygons (db::Coord) const
{
  return new EmptyRegion ();
}

RegionDelegate *
EmptyEdgePairs::processed_to_polygons (const EdgePairToPolygonProcessorBase &) const
{
  return new EmptyRegion ();
}

EdgesDelegate *
EmptyEdgePairs::processed_to_edges (const EdgePairToEdgeProcessorBase &) const
{
  return new EmptyEdges ();
}

EdgesDelegate *
EmptyEdgePairs::edges () const
{
  return new EmptyEdges ();
}

EdgesDelegate *
EmptyEdgePairs::first_edges () const
{
  return new EmptyEdges ();
}

EdgesDelegate *
EmptyEdgePairs::second_edges () const
{
  return new EmptyEdges ();
}

EdgePairsDelegate *
EmptyEdgePairs::add_in_place (const EdgePairs &other)
{
  return add (other);
}

EdgePairsDelegate *
EmptyEdgePairs::add (const EdgePairs &other) const
{
  return other.delegate ()->clone ();
}

bool 
EmptyEdgePairs::equals (const EdgePairs &other) const
{
  return other.empty ();
}

bool 
EmptyEdgePairs::less (const EdgePairs &other) const
{
  return other.empty () ? false : true;
}

}


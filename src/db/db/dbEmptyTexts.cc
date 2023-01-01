
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


#include "dbEmptyTexts.h"
#include "dbEmptyRegion.h"
#include "dbEmptyEdges.h"
#include "dbTexts.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------

EmptyTexts::EmptyTexts ()
{
  //  .. nothing yet ..
}

EmptyTexts::EmptyTexts (const EmptyTexts &other)
  : TextsDelegate (other)
{
  // .. nothing yet ..
}

TextsDelegate *
EmptyTexts::clone () const
{
  return new EmptyTexts (*this);
}

RegionDelegate *
EmptyTexts::polygons (db::Coord) const
{
  return new EmptyRegion ();
}

RegionDelegate *
EmptyTexts::processed_to_polygons (const TextToPolygonProcessorBase &) const
{
  return new EmptyRegion ();
}

EdgesDelegate *
EmptyTexts::edges () const
{
  return new EmptyEdges ();
}

TextsDelegate *
EmptyTexts::add_in_place (const Texts &other)
{
  return add (other);
}

TextsDelegate *
EmptyTexts::add (const Texts &other) const
{
  return other.delegate ()->clone ();
}

bool 
EmptyTexts::equals (const Texts &other) const
{
  return other.empty ();
}

bool 
EmptyTexts::less (const Texts &other) const
{
  return other.empty () ? false : true;
}

RegionDelegate *
EmptyTexts::pull_interacting (const Region &) const
{
  return new EmptyRegion ();
}

TextsDelegate *
EmptyTexts::selected_interacting (const Region &) const
{
  return new EmptyTexts ();
}

TextsDelegate *
EmptyTexts::selected_not_interacting (const Region &) const
{
  return new EmptyTexts ();
}

}


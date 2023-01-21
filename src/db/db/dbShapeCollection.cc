
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


#include "dbShapeCollection.h"
#include "dbPropertiesRepository.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------

DeepShapeCollectionDelegateBase::DeepShapeCollectionDelegateBase ()
{
  //  .. nothing yet ..
}

DeepShapeCollectionDelegateBase::DeepShapeCollectionDelegateBase (const DeepShapeCollectionDelegateBase &other)
{
  m_deep_layer = other.m_deep_layer.copy ();
}

DeepShapeCollectionDelegateBase &
DeepShapeCollectionDelegateBase::operator= (const DeepShapeCollectionDelegateBase &other)
{
  if (this != &other) {
    m_deep_layer = other.m_deep_layer.copy ();
  }
  return *this;
}

void
DeepShapeCollectionDelegateBase::apply_property_translator (const db::PropertiesTranslator &pt)
{
  db::Layout &layout = m_deep_layer.layout ();
  for (auto c = layout.begin (); c != layout.end (); ++c) {

    db::Shapes &shapes = c->shapes (m_deep_layer.layer ());
    if ((shapes.type_mask () & ShapeIterator::Properties) != 0) {

      //  properties are present - need to translate them

      db::Shapes new_shapes (shapes.is_editable ());
      shapes.swap (new_shapes);

      shapes.assign (new_shapes, pt);

    }

  }
}

// -------------------------------------------------------------------------------------------------------------
//  ShapeCollection implementation

const db::PropertiesRepository &
ShapeCollection::properties_repository () const
{
  static db::PropertiesRepository empty_prop_repo;
  const db::PropertiesRepository *r = get_delegate () ? get_delegate ()->properties_repository () : 0;
  return *(r ? r : &empty_prop_repo);
}

db::PropertiesRepository &
ShapeCollection::properties_repository ()
{
  db::PropertiesRepository *r = get_delegate () ? get_delegate ()->properties_repository () : 0;
  tl_assert (r != 0);
  return *r;
}

bool
ShapeCollection::has_properties_repository () const
{
  return get_delegate () && get_delegate ()->properties_repository ();
}

void
ShapeCollection::apply_property_translator (const db::PropertiesTranslator &pt)
{
  if (get_delegate ()) {
    get_delegate ()->apply_property_translator (pt);
  }
}

}

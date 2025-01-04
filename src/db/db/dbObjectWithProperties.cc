
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "dbObjectWithProperties.h"
#include "dbUserObject.h"
#include "dbText.h"
#include "dbPolygon.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbPoint.h"
#include "dbEdge.h"
#include "dbEdgePair.h"

namespace db
{

//  explicit instantiations

template class db::object_with_properties<db::Box>;
template class db::object_with_properties<db::UserObject>;
template class db::object_with_properties<db::Polygon>;
template class db::object_with_properties<db::SimplePolygon>;
template class db::object_with_properties<db::Path>;
template class db::object_with_properties<db::Text>;
template class db::object_with_properties<db::Point>;
template class db::object_with_properties<db::Edge>;
template class db::object_with_properties<db::EdgePair>;

template class db::object_with_properties<db::DBox>;
template class db::object_with_properties<db::DUserObject>;
template class db::object_with_properties<db::DPolygon>;
template class db::object_with_properties<db::DSimplePolygon>;
template class db::object_with_properties<db::DPath>;
template class db::object_with_properties<db::DText>;
template class db::object_with_properties<db::DPoint>;
template class db::object_with_properties<db::DEdge>;
template class db::object_with_properties<db::DEdgePair>;

}

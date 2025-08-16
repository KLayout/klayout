
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

namespace tl
{

template <class T> bool _test_extractor_impl (tl::Extractor &ex, db::object_with_properties<T> &p)
{
  if (! tl::test_extractor_impl (ex, (T &) p)) {
    return false;
  }

  if (ex.test ("props")) {

    if (! ex.test ("=")) {
      return false;
    }

    tl::Variant v;
    if (! tl::test_extractor_impl (ex, v)) {
      return false;
    }
    if (! v.is_array ()) {
      return false;
    }

    db::PropertiesSet props;
    for (auto i = v.begin_array (); i != v.end_array (); ++i) {
      props.insert (i->first, i->second);
    }

    p.properties_id (db::properties_id (props));

  }

  return true;
}

template <class T> void _extractor_impl (tl::Extractor &ex, db::object_with_properties<T> &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (tl::to_string (tr ("Expected a shape specification with properties")));
  }
}

template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Box> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::UserObject> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Polygon> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::SimplePolygon> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Path> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Text> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Point> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Edge> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::EdgePair> &p) { return _test_extractor_impl (ex, p); }

template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DBox> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DUserObject> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DPolygon> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DSimplePolygon> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DPath> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DText> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DPoint> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DEdge> &p) { return _test_extractor_impl (ex, p); }
template <> bool test_extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DEdgePair> &p) { return _test_extractor_impl (ex, p); }

template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Box> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::UserObject> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Polygon> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::SimplePolygon> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Path> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Text> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Point> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::Edge> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::EdgePair> &p) { _extractor_impl (ex, p); }

template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DBox> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DUserObject> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DPolygon> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DSimplePolygon> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DPath> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DText> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DPoint> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DEdge> &p) { _extractor_impl (ex, p); }
template <> void extractor_impl (tl::Extractor &ex, db::object_with_properties<db::DEdgePair> &p) { _extractor_impl (ex, p); }

}


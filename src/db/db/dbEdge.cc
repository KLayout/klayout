
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbEdge.h"

namespace db
{

/**
 *  @brief Computes the gcd of two numbers
 */
template <class C>
inline C gcd (C a, C b)
{
  while (b != 0) {
    a %= b;
    std::swap (a, b);
  }
  return a;
}

db::Coord div_exact (db::Coord a, db::coord_traits<db::Coord>::area_type b, db::coord_traits<db::Coord>::area_type d)
{
  if (a < 0) {
    return -db::Coord ((__int128 (-a) * __int128 (b) + __int128 (d / 2)) / __int128 (d));
  } else {
    return db::Coord ((__int128 (a) * __int128 (b) + __int128 ((d - 1) / 2)) / __int128 (d));
  }
}

}

namespace tl
{

template<> void extractor_impl (tl::Extractor &ex, db::Edge &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (tl::to_string (QObject::tr ("Expected an edge specification")));
  }
}

template<> void extractor_impl (tl::Extractor &ex, db::DEdge &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (tl::to_string (QObject::tr ("Expected an edge specification")));
  }
}

template<class C> bool _test_extractor_impl (tl::Extractor &ex, db::edge<C> &e)
{
  typedef db::point<C> point_type;

  if (ex.test ("(")) {

    point_type p1, p2;
    ex.read (p1);
    ex.expect (";");
    ex.read (p2);

    e = db::edge<C> (p1, p2);

    ex.expect (")");

    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::Edge &e)
{
  return _test_extractor_impl (ex, e);
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::DEdge &e)
{
  return _test_extractor_impl (ex, e);
}

} // namespace tl


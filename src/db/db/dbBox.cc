
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


#include "dbBox.h"
#include "tlString.h"
#include "tlVariant.h"

namespace tl
{

template<> void extractor_impl (tl::Extractor &ex, db::Box &b)
{
  if (! test_extractor_impl (ex, b)) {
    ex.error (tl::to_string (tr ("Expected an box specification")));
  }
}

template<> void extractor_impl (tl::Extractor &ex, db::DBox &b)
{
  if (! test_extractor_impl (ex, b)) {
    ex.error (tl::to_string (tr ("Expected an box specification")));
  }
}

template<class C> bool _test_extractor_impl (tl::Extractor &ex, db::box<C> &b)
{
  typedef db::point<C> point_type;

  if (ex.test ("(")) {

    if (ex.test (")")) {
      b = db::box<C> ();
    } else {

      point_type p1, p2;
      ex.read (p1);
      ex.expect (";");
      ex.read (p2);

      b = db::box<C> (p1, p2);

      ex.expect (")");

    }

    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::Box &b)
{
  return _test_extractor_impl (ex, b);
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::DBox &b)
{
  return _test_extractor_impl (ex, b);
}

}



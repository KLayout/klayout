
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


#include "dbEdgePair.h"

namespace tl
{

template<> void extractor_impl (tl::Extractor &ex, db::EdgePair &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (tl::to_string (tr ("Expected an edge specification")));
  }
}

template<> void extractor_impl (tl::Extractor &ex, db::DEdgePair &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (tl::to_string (tr ("Expected an edge specification")));
  }
}

template<class C> bool _test_extractor_impl (tl::Extractor &ex, db::edge_pair<C> &e)
{
  typedef db::edge<C> edge_type;
  tl::Extractor ex_saved = ex;

  edge_type e1, e2;

  if (ex.try_read (e1)) {

    bool symmetric = false;
    if (ex.test ("|")) {
      symmetric = true;
    } else if (! ex.test ("/")) {
      ex = ex_saved;
      return false;
    }

    if (! ex.try_read (e2)) {
      ex = ex_saved;
      return false;
    }

    e = db::edge_pair<C> (e1, e2, symmetric);
    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::EdgePair &e)
{
  return _test_extractor_impl (ex, e);
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::DEdgePair &e)
{
  return _test_extractor_impl (ex, e);
}

} // namespace tl


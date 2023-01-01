
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


#include "dbVector.h"

// ----------------------------------------------------------------
//  Implementation of the custom extractors

namespace {

  template <class C>
  bool _test_extractor_impl (tl::Extractor &ex, db::vector<C> &p)
  {
    //  TODO: this is not really a "test": we should move back if we did not
    //  receive a ",".
    C x = 0;
    if (ex.try_read (x)) {
      ex.expect (",");
      C y = 0;
      ex.read (y);
      p = db::vector<C> (x, y);
      return true;
    } else {
      return false;
    }
  }

  template <class C>
  void _extractor_impl (tl::Extractor &ex, db::vector<C> &p)
  {
    if (! _test_extractor_impl (ex, p)) {
      ex.error (tl::to_string (tr ("Expected a vector specification")));
    }
  }

}

namespace tl 
{
  
template <> 
void 
extractor_impl (tl::Extractor &ex, db::Vector &p)
{
  _extractor_impl (ex, p);
}

template <> 
void 
extractor_impl (tl::Extractor &ex, db::DVector &p)
{
  _extractor_impl (ex, p);
}

template <> 
bool 
test_extractor_impl (tl::Extractor &ex, db::Vector &p)
{
  return _test_extractor_impl (ex, p);
}

template <> 
bool 
test_extractor_impl (tl::Extractor &ex, db::DVector &p)
{
  return _test_extractor_impl (ex, p);
}

} // namespace tl


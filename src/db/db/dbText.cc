
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "dbText.h"

namespace db
{

StringRef::~StringRef ()
{
  if (mp_rep) {
    mp_rep->unregister_ref (this);
  }
}

}

namespace tl
{

template<> void extractor_impl (tl::Extractor &ex, db::Text &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (tl::to_string (tr ("Expected a text specification")));
  }
}

template<> void extractor_impl (tl::Extractor &ex, db::DText &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (tl::to_string (tr ("Expected a text specification")));
  }
}


template<class C> bool _test_extractor_impl (tl::Extractor &ex, db::text<C> &t)
{
  if (ex.test ("(")) {

    std::string s;
    ex.read_word_or_quoted (s);
    t.string (s);

    ex.expect (",");

    typename db::text<C>::trans_type tt;
    ex.read (tt);
    t.trans (tt);

    ex.expect (")");

    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::Text &p)
{
  return _test_extractor_impl (ex, p);
}

template<> bool test_extractor_impl (tl::Extractor &ex, db::DText &p)
{
  return _test_extractor_impl (ex, p);
}

}


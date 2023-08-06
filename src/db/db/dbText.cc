
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


#include "dbText.h"

namespace db
{

static char halign2code (db::HAlign ha)
{
  if (ha == db::HAlignCenter) {
    return 'c';
  } else if (ha == db::HAlignLeft) {
    return 'l';
  } else if (ha == db::HAlignRight) {
    return 'r';
  } else {
    return 0;
  }
}

static db::HAlign extract_halign (tl::Extractor &ex)
{
  if (ex.test ("c")) {
    return db::HAlignCenter;
  } else if (ex.test ("l")) {
    return db::HAlignLeft;
  } else if (ex.test ("r")) {
    return db::HAlignRight;
  } else {
    return db::NoHAlign;
  }
}

static char valign2code (db::VAlign va)
{
  if (va == db::VAlignCenter) {
    return 'c';
  } else if (va == db::VAlignBottom) {
    return 'b';
  } else if (va == db::VAlignTop) {
    return 't';
  } else {
    return 0;
  }
}

static db::VAlign extract_valign (tl::Extractor &ex)
{
  if (ex.test ("c")) {
    return db::VAlignCenter;
  } else if (ex.test ("t")) {
    return db::VAlignTop;
  } else if (ex.test ("b")) {
    return db::VAlignBottom;
  } else {
    return db::NoVAlign;
  }
}

// ----------------------------------------------------------------------------
//  StringRef implementation

StringRef::~StringRef ()
{
  if (mp_rep) {
    mp_rep->unregister_ref (this);
  }
}

// ----------------------------------------------------------------------------
//  text implementation

template <class C>
std::string text<C>::to_string (double dbu) const
{
  std::string s = std::string ("(") + tl::to_quoted_string (string ()) + "," + m_trans.to_string (dbu) + ")";

  if (size () > 0) {
    s += " s=";
    s += tl::to_string (size ());
  }

  if (font () >= 0) {
    s += " f=";
    s += tl::to_string (int (font ()));
  }

  char c;
  c = halign2code (halign ());
  if (c) {
    s += " ha=";
    s += c;
  }
  c = valign2code (valign ());
  if (c) {
    s += " va=";
    s += c;
  }

  return s;
}

template class DB_PUBLIC text<Coord>;
template class DB_PUBLIC text<DCoord>;

}

namespace tl
{

template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Text &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (tl::to_string (tr ("Expected a text specification")));
  }
}

template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DText &p)
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

    if (ex.test ("s=")) {
      C size = 0;
      ex.read (size);
      t.size (size);
    }

    if (ex.test ("f=")) {
      int font = -1;
      ex.read (font);
      t.font (db::Font (font));
    }

    if (ex.test ("ha=")) {
      db::HAlign ha = db::extract_halign (ex);
      t.halign (ha);
    }

    if (ex.test ("va=")) {
      db::VAlign va = db::extract_valign (ex);
      t.valign (va);
    }

    return true;

  } else {
    return false;
  }
}

template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Text &p)
{
  return _test_extractor_impl (ex, p);
}

template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DText &p)
{
  return _test_extractor_impl (ex, p);
}

}


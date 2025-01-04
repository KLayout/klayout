
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


#include "dbText.h"
#include "tlThreads.h"

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
//  StringRepository implementation

static StringRepository s_repository;
static StringRepository *sp_repository = 0;
static tl::Mutex s_repository_lock;

StringRepository *
StringRepository::instance ()
{
  return sp_repository;
}

StringRepository::StringRepository ()
{
  sp_repository = this;
}

StringRepository::~StringRepository ()
{
  if (sp_repository == this) {
    sp_repository = 0;
  }

  for (std::set<StringRef *>::const_iterator s = m_string_refs.begin (); s != m_string_refs.end (); ++s) {
    delete *s;
  }
}

const StringRef *
StringRepository::create_string_ref ()
{
  tl::MutexLocker locker (&s_repository_lock);
  StringRef *ref = new StringRef ();
  m_string_refs.insert (ref);
  return ref;
}

void
StringRepository::unregister_ref (StringRef *ref)
{
  tl::MutexLocker locker (&s_repository_lock);
  if (! m_string_refs.empty ()) {
    m_string_refs.erase (ref);
  }
}

// ----------------------------------------------------------------------------
//  StringRef implementation

static tl::Mutex s_ref_lock;

StringRef::~StringRef ()
{
  if (StringRepository::instance ()) {
    StringRepository::instance ()->unregister_ref (this);
  }
}

void
StringRef::add_ref ()
{
  tl::MutexLocker locker (&s_ref_lock);
  ++m_ref_count;
}

void
StringRef::remove_ref ()
{
  tl::MutexLocker locker (&s_ref_lock);
  --m_ref_count;
  if (m_ref_count == 0) {
    delete this;
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

template class text<Coord>;
template class text<DCoord>;

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


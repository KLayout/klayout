
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


#include "dbTrans.h"
#include "tlInternational.h"

// ----------------------------------------------------------------
//  Implementation of the custom extractors

namespace {

  template <class C>
  bool _test_extractor_impl (tl::Extractor &ex, db::simple_trans<C> &t)
  {
    bool any = false;
    db::FTrans f;
    db::vector<C> p;
    while (true) {
      if (ex.try_read (f)) {
        any = true;
      } else if (ex.try_read (p)) {
        any = true;
      } else {
        if (any) {
          t = db::simple_trans<C> (f.rot (), p);
        }
        return any;
      }
    }
  }

  template <class C>
  void _extractor_impl (tl::Extractor &ex, db::simple_trans<C> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (tl::to_string (tr ("Expected a transformation specification")));
    }
  }

  template <class C>
  bool _test_extractor_impl (tl::Extractor &ex, db::disp_trans<C> &t)
  {
    bool any = false;
    db::vector<C> p;
    while (true) {
      if (ex.try_read (p)) {
        any = true;
      } else {
        if (any) {
          t = db::disp_trans<C> (p);
        }
        return any;
      }
    }
  }

  template <class C>
  void _extractor_impl (tl::Extractor &ex, db::disp_trans<C> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (tl::to_string (tr ("Expected a transformation specification")));
    }
  }

  template <class I, class F, class R>
  bool _test_extractor_impl (tl::Extractor &ex, db::complex_trans<I, F, R> &t)
  {
    t = db::complex_trans<I, F, R> ();
    bool any = false;
    while (true) {
      db::vector<F> p;
      if (ex.test ("*")) {
        double f = 1.0;
        ex.read (f);
        t.mag (f);
        any = true;
      } else if (ex.try_read (p)) {
        t.disp (p);
        any = true;
      } else if (ex.test ("m")) {
        double a = 0.0;
        ex.read (a);
        t.mirror (true);
        t.angle (a * 2.0);
        any = true;
      } else if (ex.test ("r")) {
        double a = 0.0;
        ex.read (a);
        t.mirror (false);
        t.angle (a);
        any = true;
      } else {
        break;
      }
    }
    return any;
  }

  template <class I, class F, class R>
  void _extractor_impl (tl::Extractor &ex, db::complex_trans<I, F, R> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (tl::to_string (tr ("Expected transformation specification")));
    }
  }

  template <class C1, class C2>
  bool _test_extractor_impl (tl::Extractor &ex, db::combined_trans<C1, C2> &t)
  {
    bool any = false;
    C1 t1;
    C2 t2;
    while (true) {
      if (ex.try_read (t1)) {
        any = true;
      } else if (ex.try_read (t2)) {
        any = true;
      } else {
        if (any) {
          t = db::combined_trans<C1, C2> (t1, t2);
        }
        return any;
      }
    }
  }

  template <class C1, class C2>
  void _extractor_impl (tl::Extractor &ex, db::combined_trans<C1, C2> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (tl::to_string (tr ("Expected transformation/magnification specification")));
    }
  }

}

namespace tl
{

template <>
void DB_PUBLIC 
extractor_impl (tl::Extractor &ex, db::FTrans &t)
{
  if (! test_extractor_impl (ex, t)) {
    ex.error (tl::to_string (tr ("Expected rotation/mirror code (r0,r90,r180,r270,m0,m45,m90,m135)")));
  }
}

template <>
void DB_PUBLIC 
extractor_impl (tl::Extractor &ex, db::Trans &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (tl::Extractor &ex, db::DTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (tl::Extractor &ex, db::Disp &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (tl::Extractor &ex, db::DDisp &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (tl::Extractor &ex, db::CplxTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
DB_PUBLIC void 
extractor_impl (tl::Extractor &ex, db::ICplxTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
DB_PUBLIC void 
extractor_impl (tl::Extractor &ex, db::DCplxTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
DB_PUBLIC void
extractor_impl (tl::Extractor &ex, db::VCplxTrans &t)
{
  _extractor_impl (ex, t);
}


template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::FTrans &t)
{
  if (ex.test ("r0")) {
    t = db::FTrans (db::FTrans::r0);
    return true;
  } else if (ex.test ("r90")) {
    t = db::FTrans (db::FTrans::r90);
    return true;
  } else if (ex.test ("r180")) {
    t = db::FTrans (db::FTrans::r180);
    return true;
  } else if (ex.test ("r270")) {
    t = db::FTrans (db::FTrans::r270);
    return true;
  } else if (ex.test ("m0")) {
    t = db::FTrans (db::FTrans::m0);
    return true;
  } else if (ex.test ("m45")) {
    t = db::FTrans (db::FTrans::m45);
    return true;
  } else if (ex.test ("m90")) {
    t = db::FTrans (db::FTrans::m90);
    return true;
  } else if (ex.test ("m135")) {
    t = db::FTrans (db::FTrans::m135);
    return true;
  } else {
    return false;
  }
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::Trans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::DTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::Disp &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::DDisp &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::CplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::ICplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (tl::Extractor &ex, db::DCplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool
test_extractor_impl (tl::Extractor &ex, db::VCplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

} // namespace tl


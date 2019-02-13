
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "dbRegion.h"
#include "dbOriginalLayerRegion.h"
#include "dbEmptyRegion.h"
#include "dbFlatRegion.h"
#include "dbDeepRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  Region implementation

Region::Region ()
  : mp_delegate (new EmptyRegion ())
{
  //  .. nothing yet ..
}

Region::Region (RegionDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

Region::Region (const Region &other)
  : mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

Region::~Region ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

Region &Region::operator= (const Region &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone (), false);
  }
  return *this;
}

Region::Region (const RecursiveShapeIterator &si)
{
  mp_delegate = new OriginalLayerRegion (si);
}

Region::Region (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics)
{
  mp_delegate = new OriginalLayerRegion (si, trans, merged_semantics);
}

Region::Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
{
  mp_delegate = new DeepRegion (si, dss, area_ratio, max_vertex_count);
}

Region::Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics, double area_ratio, size_t max_vertex_count)
{
  mp_delegate = new DeepRegion (si, dss, trans, merged_semantics, area_ratio, max_vertex_count);
}

const db::RecursiveShapeIterator &
Region::iter () const
{
  static db::RecursiveShapeIterator def_iter;
  const db::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void
Region::set_delegate (RegionDelegate *delegate, bool keep_attributes)
{
  if (delegate != mp_delegate) {
    if (keep_attributes && delegate && mp_delegate) {
      //  copy the basic attributes like #threads etc.
      delegate->RegionDelegate::operator= (*mp_delegate);
    }
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

void
Region::clear ()
{
  set_delegate (new EmptyRegion ());
}

void
Region::reserve (size_t n)
{
  flat_region ()->reserve (n);
}

template <class T>
Region &Region::transform (const T &trans)
{
  flat_region ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC Region &Region::transform (const db::ICplxTrans &);
template DB_PUBLIC Region &Region::transform (const db::Trans &);
template DB_PUBLIC Region &Region::transform (const db::Disp &);

template <class Sh>
void Region::insert (const Sh &shape)
{
  flat_region ()->insert (shape);
}

template DB_PUBLIC void Region::insert (const db::Box &);
template DB_PUBLIC void Region::insert (const db::SimplePolygon &);
template DB_PUBLIC void Region::insert (const db::Polygon &);
template DB_PUBLIC void Region::insert (const db::Path &);

void Region::insert (const db::Shape &shape)
{
  flat_region ()->insert (shape);
}

template <class T>
void Region::insert (const db::Shape &shape, const T &trans)
{
  flat_region ()->insert (shape, trans);
}

template DB_PUBLIC void Region::insert (const db::Shape &, const db::ICplxTrans &);
template DB_PUBLIC void Region::insert (const db::Shape &, const db::Trans &);
template DB_PUBLIC void Region::insert (const db::Shape &, const db::Disp &);

FlatRegion *
Region::flat_region ()
{
  FlatRegion *region = dynamic_cast<FlatRegion *> (mp_delegate);
  if (! region) {
    region = new FlatRegion ();
    if (mp_delegate) {
      region->insert_seq (begin ());
    }
    if (mp_delegate) {
      region->set_is_merged (mp_delegate->is_merged ());
    }
    set_delegate (region);
  }

  return region;
}

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Region &b)
  {
    db::Polygon p;

    if (! ex.try_read (p)) {
      return false;
    }
    b.insert (p);

    while (ex.test (";")) {
      ex.read (p);
      b.insert (p);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Region &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (tr ("Expected an region collection specification")));
    }
  }
}


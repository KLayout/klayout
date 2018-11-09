
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

#include "dbEdges.h"
#include "dbOriginalLayerEdges.h"
#include "dbEmptyEdges.h"
#include "dbFlatEdges.h"
#include "dbRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  Edges implementation

Edges::Edges ()
  : mp_delegate (new EmptyEdges ())
{
  //  .. nothing yet ..
}

Edges::Edges (EdgesDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

Edges::Edges (const Edges &other)
  : mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

Edges::~Edges ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

Edges &Edges::operator= (const Edges &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone ());
  }
  return *this;
}

Edges::Edges (const RecursiveShapeIterator &si, bool as_edges)
{
  if (! as_edges) {
    mp_delegate = new OriginalLayerEdges (si);
  } else {
    FlatEdges *fe = new FlatEdges ();
    mp_delegate = fe;
    for (RecursiveShapeIterator s = si; ! s.at_end (); ++s) {
      fe->insert (s.shape (), s.trans ());
    }
  }
}

Edges::Edges (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool as_edges, bool merged_semantics)
{
  if (! as_edges) {
    mp_delegate = new OriginalLayerEdges (si, trans, merged_semantics);
  } else {
    FlatEdges *fe = new FlatEdges ();
    fe->set_merged_semantics (merged_semantics);
    mp_delegate = fe;
    for (RecursiveShapeIterator s = si; ! s.at_end (); ++s) {
      fe->insert (s.shape (), trans * s.trans ());
    }
  }
}

const db::RecursiveShapeIterator &
Edges::iter () const
{
  static db::RecursiveShapeIterator def_iter;
  const db::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void
Edges::set_delegate (EdgesDelegate *delegate)
{
  if (delegate != mp_delegate) {
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

void
Edges::clear ()
{
  set_delegate (new EmptyEdges ());
}

void
Edges::reserve (size_t n)
{
  flat_edges ()->reserve (n);
}

void Edges::extended (Region &output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  output.set_delegate (mp_delegate->extended (ext_b, ext_e, ext_o, ext_i, join));
}

template <class T>
Edges &Edges::transform (const T &trans)
{
  flat_edges ()->transform (trans);
  return *this;
}

//  explicit instantiations
template Edges &Edges::transform (const db::ICplxTrans &);
template Edges &Edges::transform (const db::Trans &);
template Edges &Edges::transform (const db::Disp &);

template <class Sh>
void Edges::insert (const Sh &shape)
{
  flat_edges ()->insert (shape);
}

template void Edges::insert (const db::Box &);
template void Edges::insert (const db::SimplePolygon &);
template void Edges::insert (const db::Polygon &);
template void Edges::insert (const db::Path &);
template void Edges::insert (const db::Edge &);

void Edges::insert (const db::Shape &shape)
{
  flat_edges ()->insert (shape);
}

template <class T>
void Edges::insert (const db::Shape &shape, const T &trans)
{
  flat_edges ()->insert (shape, trans);
}

template void Edges::insert (const db::Shape &, const db::ICplxTrans &);
template void Edges::insert (const db::Shape &, const db::Trans &);
template void Edges::insert (const db::Shape &, const db::Disp &);

FlatEdges *
Edges::flat_edges ()
{
  FlatEdges *edges = dynamic_cast<FlatEdges *> (mp_delegate);
  if (! edges) {
    edges = new FlatEdges ();
    if (mp_delegate) {
      edges->EdgesDelegate::operator= (*mp_delegate);
      edges->insert_seq (begin ());
    }
    set_delegate (edges);
  }

  return edges;
}

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Edges &b)
  {
    db::Edge p;

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

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Edges &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (tr ("Expected an edge set specification")));
    }
  }
}


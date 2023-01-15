
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

#include "dbEdges.h"
#include "dbDeepEdges.h"
#include "dbOriginalLayerEdges.h"
#include "dbEmptyEdges.h"
#include "dbMutableEdges.h"
#include "dbFlatEdges.h"
#include "dbEdgesUtils.h"
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
  : db::ShapeCollection (), mp_delegate (other.mp_delegate->clone ())
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
    set_delegate (other.mp_delegate->clone (), false);
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

Edges::Edges (const RecursiveShapeIterator &si, DeepShapeStore &dss, bool as_edges)
{
  mp_delegate = new DeepEdges (si, dss, as_edges);
}

Edges::Edges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges, bool merged_semantics)
{
  mp_delegate = new DeepEdges (si, dss, trans, as_edges, merged_semantics);
}

const db::RecursiveShapeIterator &
Edges::iter () const
{
  static db::RecursiveShapeIterator def_iter;
  const db::RecursiveShapeIterator *i = mp_delegate ? mp_delegate->iter () : 0;
  return *(i ? i : &def_iter);
}

const db::PropertiesRepository &
Edges::properties_repository () const
{
  static db::PropertiesRepository empty_prop_repo;
  const db::PropertiesRepository *r = delegate () ? delegate ()->properties_repository () : 0;
  return *(r ? r : &empty_prop_repo);
}

db::PropertiesRepository &
Edges::properties_repository ()
{
  db::PropertiesRepository *r = delegate () ? delegate ()->properties_repository () : 0;
  tl_assert (r != 0);
  return *r;
}

void
Edges::set_delegate (EdgesDelegate *delegate, bool keep_attributes)
{
  if (delegate != mp_delegate) {
    if (keep_attributes && mp_delegate && delegate) {
      //  copy attributes (threads, min_coherence etc.) from old to new
      *delegate = *mp_delegate;
    }
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
  mutable_edges ()->reserve (n);
}

void Edges::processed (Region &output, const EdgeToPolygonProcessorBase &filter) const
{
  output.set_delegate (mp_delegate->processed_to_polygons (filter));
}

void Edges::pull_interacting (Region &output, const Region &other) const
{
  output = Region (mp_delegate->pull_interacting (other));
}

void Edges::extended (Region &output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  output.set_delegate (mp_delegate->extended (ext_b, ext_e, ext_o, ext_i, join));
}

Edges Edges::start_segments (length_type length, double fraction) const
{
  return Edges (mp_delegate->processed (EdgeSegmentSelector (-1, length, fraction)));
}

Edges Edges::end_segments (length_type length, double fraction) const
{
  return Edges (mp_delegate->processed (EdgeSegmentSelector (1, length, fraction)));
}

Edges Edges::centers (length_type length, double fraction) const
{
  return Edges (mp_delegate->processed (EdgeSegmentSelector (0, length, fraction)));
}

void
Edges::flatten ()
{
  mutable_edges ()->flatten ();
}

template <class T>
Edges &Edges::transform (const T &trans)
{
  mutable_edges ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC Edges &Edges::transform (const db::ICplxTrans &);
template DB_PUBLIC Edges &Edges::transform (const db::Trans &);
template DB_PUBLIC Edges &Edges::transform (const db::Disp &);
template DB_PUBLIC Edges &Edges::transform (const db::IMatrix2d &);
template DB_PUBLIC Edges &Edges::transform (const db::IMatrix3d &);

template <class Sh>
void Edges::insert (const Sh &shape)
{
  mutable_edges ()->insert (shape);
}

template DB_PUBLIC void Edges::insert (const db::Box &);
template DB_PUBLIC void Edges::insert (const db::SimplePolygon &);
template DB_PUBLIC void Edges::insert (const db::Polygon &);
template DB_PUBLIC void Edges::insert (const db::Path &);
template DB_PUBLIC void Edges::insert (const db::Edge &);

void Edges::insert (const db::Shape &shape)
{
  mutable_edges ()->insert (shape);
}

template <class T>
void Edges::insert (const db::Shape &shape, const T &trans)
{
  mutable_edges ()->insert (shape, trans);
}

template DB_PUBLIC void Edges::insert (const db::Shape &, const db::ICplxTrans &);
template DB_PUBLIC void Edges::insert (const db::Shape &, const db::Trans &);
template DB_PUBLIC void Edges::insert (const db::Shape &, const db::Disp &);

MutableEdges *
Edges::mutable_edges ()
{
  MutableEdges *edges = dynamic_cast<MutableEdges *> (mp_delegate);
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

    if (ex.at_end ()) {
      return true;
    }
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


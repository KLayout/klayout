
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "dbCommon.h"

#include "dbEdgePairs.h"
#include "dbEmptyEdgePairs.h"
#include "dbFlatEdgePairs.h"
#include "dbDeepEdgePairs.h"
#include "dbOriginalLayerEdgePairs.h"
#include "dbEdges.h"
#include "dbRegion.h"

#include "tlVariant.h"

#include <sstream>

namespace db
{

EdgePairs::EdgePairs ()
  : mp_delegate (new EmptyEdgePairs ())
{
  //  .. nothing yet ..
}

EdgePairs::~EdgePairs ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

EdgePairs::EdgePairs (EdgePairsDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

EdgePairs::EdgePairs (const EdgePairs &other)
  : gsi::ObjectBase (), mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

EdgePairs &EdgePairs::operator= (const EdgePairs &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone ());
  }
  return *this;
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si)
{
  mp_delegate = new OriginalLayerEdgePairs (si);
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  mp_delegate = new OriginalLayerEdgePairs (si, trans);
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss)
{
  mp_delegate = new DeepEdgePairs (si, dss);
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans)
{
  mp_delegate = new DeepEdgePairs (si, dss, trans);
}

template <class Sh>
void EdgePairs::insert (const Sh &shape)
{
  flat_edge_pairs ()->insert (shape);
}

template DB_PUBLIC void EdgePairs::insert (const db::EdgePair &);

void EdgePairs::insert (const db::Shape &shape)
{
  flat_edge_pairs ()->insert (shape);
}

template <class T>
void EdgePairs::insert (const db::Shape &shape, const T &trans)
{
  flat_edge_pairs ()->insert (shape, trans);
}

template DB_PUBLIC void EdgePairs::insert (const db::Shape &, const db::ICplxTrans &);
template DB_PUBLIC void EdgePairs::insert (const db::Shape &, const db::Trans &);
template DB_PUBLIC void EdgePairs::insert (const db::Shape &, const db::Disp &);

void EdgePairs::clear ()
{
  set_delegate (new EmptyEdgePairs ());
}

void EdgePairs::reserve (size_t n)
{
  flat_edge_pairs ()->reserve (n);
}

template <class T>
EdgePairs &EdgePairs::transform (const T &trans)
{
  flat_edge_pairs ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::ICplxTrans &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::Trans &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::Disp &);

const db::RecursiveShapeIterator &
EdgePairs::iter () const
{
  static db::RecursiveShapeIterator def_iter;
  const db::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void EdgePairs::polygons (Region &output, db::Coord e) const
{
  output.set_delegate (mp_delegate->polygons (e));
}

void EdgePairs::edges (Edges &output) const
{
  output.set_delegate (mp_delegate->edges ());
}

void EdgePairs::first_edges (Edges &output) const
{
  output.set_delegate (mp_delegate->first_edges ());
}

void EdgePairs::second_edges (Edges &output) const
{
  output.set_delegate (mp_delegate->second_edges ());
}

void EdgePairs::set_delegate (EdgePairsDelegate *delegate)
{
  if (delegate != mp_delegate) {
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

FlatEdgePairs *EdgePairs::flat_edge_pairs ()
{
  FlatEdgePairs *edge_pairs = dynamic_cast<FlatEdgePairs *> (mp_delegate);
  if (! edge_pairs) {
    edge_pairs = new FlatEdgePairs ();
    if (mp_delegate) {
      edge_pairs->EdgePairsDelegate::operator= (*mp_delegate);
      edge_pairs->insert_seq (begin ());
    }
    set_delegate (edge_pairs);
  }

  return edge_pairs;
}

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::EdgePairs &b)
  {
    db::EdgePair ep;

    if (! ex.try_read (ep)) {
      return false;
    }
    b.insert (ep);

    while (ex.test (";")) {
      ex.read (ep);
      b.insert (ep);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::EdgePairs &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (tr ("Expected an edge pair collection specification")));
    }
  }
}


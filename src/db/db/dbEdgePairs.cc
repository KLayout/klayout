
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "dbLayout.h"
#include "dbWriter.h"
#include "tlStream.h"

#include "tlVariant.h"

#include <sstream>

namespace db
{

// ---------------------------------------------------------------------------------------------------
//  EdgePairs implementation

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
  : db::ShapeCollection (), mp_delegate (other.mp_delegate->clone ())
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

EdgePairs::EdgePairs (DeepShapeStore &dss)
{
  tl_assert (dss.is_singular ());
  unsigned int layout_index = 0; // singular layout index
  mp_delegate = new DeepEdgePairs (DeepLayer (&dss, layout_index, dss.layout (layout_index).insert_layer ()));
}

void
EdgePairs::write (const std::string &fn) const
{
  //  method provided for debugging purposes

  db::Layout layout;
  const db::Cell &top = layout.cell (layout.add_cell ("EDGE_PAIRS"));
  unsigned int li = layout.insert_layer (db::LayerProperties (0, 0));
  insert_into (&layout, top.cell_index (), li);

  tl::OutputStream os (fn);
  db::SaveLayoutOptions opt;
  opt.set_format_from_filename (fn);
  db::Writer writer (opt);
  writer.write (layout, os);
}

template <class Sh>
void EdgePairs::insert (const Sh &shape)
{
  mutable_edge_pairs ()->insert (shape);
}

template DB_PUBLIC void EdgePairs::insert (const db::EdgePair &);
template DB_PUBLIC void EdgePairs::insert (const db::EdgePairWithProperties &);

void EdgePairs::insert (const db::Shape &shape)
{
  mutable_edge_pairs ()->insert (shape);
}

template <class T>
void EdgePairs::insert (const db::Shape &shape, const T &trans)
{
  mutable_edge_pairs ()->insert (shape, trans);
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
  mutable_edge_pairs ()->reserve (n);
}

void EdgePairs::flatten ()
{
  mutable_edge_pairs ()->flatten ();
}

template <class T>
EdgePairs &EdgePairs::transform (const T &trans)
{
  mutable_edge_pairs ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::ICplxTrans &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::Trans &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::Disp &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::IMatrix2d &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const db::IMatrix3d &);

const db::RecursiveShapeIterator &
EdgePairs::iter () const
{
  static db::RecursiveShapeIterator def_iter;
  const db::RecursiveShapeIterator *i = mp_delegate ? mp_delegate->iter () : 0;
  return *(i ? i : &def_iter);
}

EdgePairs EdgePairs::processed (const EdgePairProcessorBase &proc) const
{
  return EdgePairs (mp_delegate->processed (proc));
}

void EdgePairs::processed (Region &output, const EdgePairToPolygonProcessorBase &proc) const
{
  output = Region (mp_delegate->processed_to_polygons (proc));
}

void EdgePairs::processed (Edges &output, const EdgePairToEdgeProcessorBase &proc) const
{
  output = Edges (mp_delegate->processed_to_edges (proc));
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

void EdgePairs::pull_interacting (Region &output, const Region &other) const
{
  output = Region (mp_delegate->pull_interacting (other));
}

void EdgePairs::pull_interacting (Edges &output, const Edges &other) const
{
  output = Edges (mp_delegate->pull_interacting (other));
}

void EdgePairs::set_delegate (EdgePairsDelegate *delegate)
{
  if (delegate != mp_delegate) {
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

MutableEdgePairs *EdgePairs::mutable_edge_pairs ()
{
  MutableEdgePairs *edge_pairs = dynamic_cast<MutableEdgePairs *> (mp_delegate);
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

    if (ex.at_end ()) {
      return true;
    }
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


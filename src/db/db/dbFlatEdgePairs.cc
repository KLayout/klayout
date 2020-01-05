
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


#include "dbFlatEdgePairs.h"
#include "dbEmptyEdgePairs.h"
#include "dbEdgePairs.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  FlatEdgePairs implementation

FlatEdgePairs::FlatEdgePairs ()
  : AsIfFlatEdgePairs (), m_edge_pairs (false)
{
  //  .. nothing yet ..
}

FlatEdgePairs::~FlatEdgePairs ()
{
  //  .. nothing yet ..
}

FlatEdgePairs::FlatEdgePairs (const FlatEdgePairs &other)
  : AsIfFlatEdgePairs (other), m_edge_pairs (false)
{
  m_edge_pairs = other.m_edge_pairs;
}

FlatEdgePairs::FlatEdgePairs (const db::Shapes &edge_pairs)
  : AsIfFlatEdgePairs (), m_edge_pairs (edge_pairs)
{
  //  .. nothing yet ..
}

void FlatEdgePairs::invalidate_cache ()
{
  invalidate_bbox ();
}

void FlatEdgePairs::reserve (size_t n)
{
  m_edge_pairs.reserve (db::EdgePair::tag (), n);
}

EdgePairsIteratorDelegate *FlatEdgePairs::begin () const
{
  return new FlatEdgePairsIterator (m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().begin (), m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ());
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatEdgePairs::begin_iter () const
{
  return std::make_pair (db::RecursiveShapeIterator (m_edge_pairs), db::ICplxTrans ());
}

bool FlatEdgePairs::empty () const
{
  return m_edge_pairs.empty ();
}

size_t FlatEdgePairs::size () const
{
  return m_edge_pairs.size ();
}

Box FlatEdgePairs::compute_bbox () const
{
  m_edge_pairs.update_bbox ();
  return m_edge_pairs.bbox ();
}

EdgePairsDelegate *
FlatEdgePairs::filter_in_place (const EdgePairFilterBase &filter)
{
  edge_pair_iterator_type pw = m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().begin ();
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (pw == m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ()) {
        m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().insert (*p);
        pw = m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ();
      } else {
        m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().replace (pw++, *p);
      }
    }
  }

  m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().erase (pw, m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ());

  return this;
}

EdgePairsDelegate *FlatEdgePairs::add (const EdgePairs &other) const
{
  std::auto_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs (*this));
  new_edge_pairs->invalidate_cache ();

  FlatEdgePairs *other_flat = dynamic_cast<FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    new_edge_pairs->raw_edge_pairs ().insert (other_flat->raw_edge_pairs ().get_layer<db::EdgePair, db::unstable_layer_tag> ().begin (), other_flat->raw_edge_pairs ().get_layer<db::EdgePair, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = new_edge_pairs->raw_edge_pairs ().size ();
    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    new_edge_pairs->raw_edge_pairs ().reserve (db::EdgePair::tag (), n);

    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_edge_pairs->raw_edge_pairs ().insert (*p);
    }

  }

  return new_edge_pairs.release ();
}

EdgePairsDelegate *FlatEdgePairs::add_in_place (const EdgePairs &other)
{
  invalidate_cache ();

  FlatEdgePairs *other_flat = dynamic_cast<FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    m_edge_pairs.insert (other_flat->raw_edge_pairs ().get_layer<db::EdgePair, db::unstable_layer_tag> ().begin (), other_flat->raw_edge_pairs ().get_layer<db::EdgePair, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = m_edge_pairs.size ();
    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    m_edge_pairs.reserve (db::EdgePair::tag (), n);

    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      m_edge_pairs.insert (*p);
    }

  }

  return this;
}

const db::EdgePair *FlatEdgePairs::nth (size_t n) const
{
  return n < m_edge_pairs.size () ? &m_edge_pairs.get_layer<db::EdgePair, db::unstable_layer_tag> ().begin () [n] : 0;
}

bool FlatEdgePairs::has_valid_edge_pairs () const
{
  return true;
}

const db::RecursiveShapeIterator *FlatEdgePairs::iter () const
{
  return 0;
}

void
FlatEdgePairs::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  db::Shapes &out = layout->cell (into_cell).shapes (into_layer);
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    out.insert (p->normalized ().to_simple_polygon (enl));
  }
}

void
FlatEdgePairs::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  layout->cell (into_cell).shapes (into_layer).insert (m_edge_pairs);
}

void
FlatEdgePairs::insert (const db::EdgePair &ep)
{
  m_edge_pairs.insert (ep);
  invalidate_cache ();
}

void
FlatEdgePairs::insert (const db::Shape &shape)
{
  if (shape.is_edge_pair ()) {

    db::EdgePair ep;
    shape.edge_pair (ep);
    insert (ep);

  }
}

}


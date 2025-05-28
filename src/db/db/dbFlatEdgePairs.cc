
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


#include "dbFlatEdgePairs.h"
#include "dbEmptyEdgePairs.h"
#include "dbEdgePairs.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  FlatEdgePairs implementation

FlatEdgePairs::FlatEdgePairs ()
  : MutableEdgePairs (), mp_edge_pairs (new db::Shapes (false))
{
  //  .. nothing yet ..
}

FlatEdgePairs::~FlatEdgePairs ()
{
  //  .. nothing yet ..
}

FlatEdgePairs::FlatEdgePairs (const FlatEdgePairs &other)
  : MutableEdgePairs (other), mp_edge_pairs (other.mp_edge_pairs)
{
  //  .. nothing yet ..
}

FlatEdgePairs::FlatEdgePairs (const db::Shapes &edge_pairs)
  : MutableEdgePairs (), mp_edge_pairs (new db::Shapes (edge_pairs))
{
  //  .. nothing yet ..
}

void FlatEdgePairs::invalidate_cache ()
{
  invalidate_bbox ();
}

void FlatEdgePairs::reserve (size_t n)
{
  mp_edge_pairs->reserve (db::EdgePair::tag (), n);
}

EdgePairsIteratorDelegate *FlatEdgePairs::begin () const
{
  return new FlatEdgePairsIterator (mp_edge_pairs.get_const ());
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatEdgePairs::begin_iter () const
{
  return std::make_pair (db::RecursiveShapeIterator (*mp_edge_pairs), db::ICplxTrans ());
}

bool FlatEdgePairs::empty () const
{
  return mp_edge_pairs->empty ();
}

size_t FlatEdgePairs::count () const
{
  return mp_edge_pairs->size ();
}

size_t FlatEdgePairs::hier_count () const
{
  return mp_edge_pairs->size ();
}

Box FlatEdgePairs::compute_bbox () const
{
  return mp_edge_pairs->bbox ();
}

EdgePairsDelegate *
FlatEdgePairs::filter_in_place (const EdgePairFilterBase &filter)
{
  //  TODO: implement property support

  db::Shapes &ep = *mp_edge_pairs;

  edge_pair_iterator_type pw = ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().begin ();
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p, p.prop_id ())) {
      if (pw == ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ()) {
        ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().insert (*p);
        pw = ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ();
      } else {
        ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().replace (pw++, *p);
      }
    }
  }

  ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().erase (pw, ep.get_layer<db::EdgePair, db::unstable_layer_tag> ().end ());

  return this;
}

EdgePairsDelegate *FlatEdgePairs::add (const EdgePairs &other) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs (*this));
  new_edge_pairs->invalidate_cache ();

  const FlatEdgePairs *other_flat = dynamic_cast<const FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    new_edge_pairs->raw_edge_pairs ().insert (other_flat->raw_edge_pairs ());

  } else {

    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = p.prop_id ();
      if (prop_id != 0) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }

  }

  return new_edge_pairs.release ();
}

EdgePairsDelegate *FlatEdgePairs::add_in_place (const EdgePairs &other)
{
  invalidate_cache ();

  db::Shapes &ep = *mp_edge_pairs;

  const FlatEdgePairs *other_flat = dynamic_cast<const FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    ep.insert (other_flat->raw_edge_pairs ());

  } else {

    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = p.prop_id ();
      if (prop_id != 0) {
        ep.insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        ep.insert (*p);
      }
    }

  }

  return this;
}

const db::EdgePair *FlatEdgePairs::nth (size_t n) const
{
  //  NOTE: this assumes that we iterate over non-property edge pairs first and then over edges with properties

  if (n >= mp_edge_pairs->size ()) {
    return 0;
  }

  const db::layer<db::EdgePair, db::unstable_layer_tag> &l = mp_edge_pairs->get_layer<db::EdgePair, db::unstable_layer_tag> ();
  if (n < l.size ()) {
    return &l.begin () [n];
  }
  n -= l.size ();

  const db::layer<db::EdgePairWithProperties, db::unstable_layer_tag> &lp = mp_edge_pairs->get_layer<db::EdgePairWithProperties, db::unstable_layer_tag> ();
  if (n < lp.size ()) {
    return &lp.begin () [n];
  }

  return 0;
}

db::properties_id_type FlatEdgePairs::nth_prop_id (size_t n) const
{
  //  NOTE: this assumes that we iterate over non-property edge pairs first and then over edges with properties

  if (n >= mp_edge_pairs->size ()) {
    return 0;
  }

  const db::layer<db::EdgePair, db::unstable_layer_tag> &l = mp_edge_pairs->get_layer<db::EdgePair, db::unstable_layer_tag> ();
  if (n < l.size ()) {
    return 0;
  }
  n -= l.size ();

  const db::layer<db::EdgePairWithProperties, db::unstable_layer_tag> &lp = mp_edge_pairs->get_layer<db::EdgePairWithProperties, db::unstable_layer_tag> ();
  if (n < lp.size ()) {
    return lp.begin () [n].properties_id ();
  }

  return 0;
}

bool FlatEdgePairs::has_valid_edge_pairs () const
{
  return true;
}

const db::RecursiveShapeIterator *FlatEdgePairs::iter () const
{
  return 0;
}

void FlatEdgePairs::apply_property_translator (const db::PropertiesTranslator &pt)
{
  if ((mp_edge_pairs->type_mask () & db::ShapeIterator::Properties) != 0) {

    db::Shapes new_edge_pairs (mp_edge_pairs->is_editable ());
    new_edge_pairs.assign (*mp_edge_pairs, pt);
    mp_edge_pairs->swap (new_edge_pairs);

    invalidate_cache ();

  }
}

void
FlatEdgePairs::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  db::Shapes &out = layout->cell (into_cell).shapes (into_layer);

  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    db::properties_id_type prop_id = p.prop_id ();
    if (prop_id != 0) {
      out.insert (db::SimplePolygonWithProperties (p->normalized ().to_simple_polygon (enl), prop_id));
    } else {
      out.insert (p->normalized ().to_simple_polygon (enl));
    }
  }
}

void
FlatEdgePairs::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  layout->cell (into_cell).shapes (into_layer).insert (*mp_edge_pairs);
}

void
FlatEdgePairs::do_insert (const db::EdgePair &ep, db::properties_id_type prop_id)
{
  if (prop_id != 0) {
    mp_edge_pairs->insert (db::EdgePairWithProperties (ep, prop_id));
  } else {
    mp_edge_pairs->insert (ep);
  }
  invalidate_cache ();
}

}



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


#include "dbDeepEdgePairs.h"
#include "dbCellGraphUtils.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"

#include <sstream>

namespace db
{

/**
 *  @brief An iterator delegate for the deep edge pair collection
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DB_PUBLIC DeepEdgePairsIterator
  : public EdgePairsIteratorDelegate
{
public:
  DeepEdgePairsIterator (const db::RecursiveShapeIterator &iter)
    : m_iter (iter)
  {
    set ();
  }

  virtual ~DeepEdgePairsIterator () { }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
    set ();
  }

  virtual const value_type *get () const
  {
    return &m_edge_pair;
  }

  virtual EdgePairsIteratorDelegate *clone () const
  {
    return new DeepEdgePairsIterator (*this);
  }

private:
  friend class EdgePairs;

  db::RecursiveShapeIterator m_iter;
  mutable value_type m_edge_pair;

  void set () const  {
    if (! m_iter.at_end ()) {
      m_iter.shape ().edge_pair (m_edge_pair);
      m_edge_pair.transform (m_iter.trans ());
    }
  }
};


DeepEdgePairs::DeepEdgePairs ()
  : AsIfFlatEdgePairs ()
{
  //  .. nothing yet ..
}

DeepEdgePairs::DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss)
  : AsIfFlatEdgePairs ()
{
  set_deep_layer (dss.create_edge_pair_layer (si));
}

DeepEdgePairs::DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans)
  : AsIfFlatEdgePairs ()
{
  set_deep_layer (dss.create_edge_pair_layer (si, trans));
}

DeepEdgePairs::DeepEdgePairs (const DeepEdgePairs &other)
  : AsIfFlatEdgePairs (other), db::DeepShapeCollectionDelegateBase (other)
{
  //  .. nothing yet ..
}

DeepEdgePairs::DeepEdgePairs (const DeepLayer &dl)
  : AsIfFlatEdgePairs ()
{
  set_deep_layer (dl);
}

DeepEdgePairs::~DeepEdgePairs ()
{
  //  .. nothing yet ..
}

EdgePairsDelegate *DeepEdgePairs::clone () const
{
  return new DeepEdgePairs (*this);
}

EdgePairsIteratorDelegate *DeepEdgePairs::begin () const
{
  return new DeepEdgePairsIterator (begin_iter ().first);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> DeepEdgePairs::begin_iter () const
{
  const db::Layout &layout = deep_layer ().layout ();
  if (layout.cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    db::RecursiveShapeIterator iter (deep_layer ().layout (), top_cell, deep_layer ().layer ());
    return std::make_pair (iter, db::ICplxTrans ());

  }
}

size_t DeepEdgePairs::size () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

std::string DeepEdgePairs::to_string (size_t nmax) const
{
  return db::AsIfFlatEdgePairs::to_string (nmax);
}

Box DeepEdgePairs::bbox () const
{
  return deep_layer ().initial_cell ().bbox (deep_layer ().layer ());
}

bool DeepEdgePairs::empty () const
{
  return begin_iter ().first.at_end ();
}

const db::EdgePair *DeepEdgePairs::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to edge pairs is available only for flat edge pair collections")));
}

bool DeepEdgePairs::has_valid_edge_pairs () const
{
  return false;
}

const db::RecursiveShapeIterator *DeepEdgePairs::iter () const
{
  return 0;
}

EdgePairsDelegate *
DeepEdgePairs::add_in_place (const EdgePairs &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepEdgePairs *other_deep = dynamic_cast <const DeepEdgePairs *> (other.delegate ());
  if (other_deep) {

    deep_layer ().add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    for (db::EdgePairs::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      shapes.insert (*p);
    }

  }

  return this;
}

EdgePairsDelegate *DeepEdgePairs::add (const EdgePairs &other) const
{
  if (other.empty ()) {
    return clone ();
  } else if (empty ()) {
    return other.delegate ()->clone ();
  } else {
    DeepEdgePairs *new_edge_pairs = dynamic_cast<DeepEdgePairs *> (clone ());
    new_edge_pairs->add_in_place (other);
    return new_edge_pairs;
  }
}

EdgePairsDelegate *DeepEdgePairs::filter_in_place (const EdgePairFilterBase &filter)
{
  //  TODO: implement
  return AsIfFlatEdgePairs::filter_in_place (filter);
}

EdgePairsDelegate *DeepEdgePairs::filtered (const EdgePairFilterBase &filter) const
{
  //  TODO: implement
  return AsIfFlatEdgePairs::filtered (filter);
}

RegionDelegate *
DeepEdgePairs::processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::EdgePair, db::Polygon, db::DeepRegion> (deep_layer (), filter);
}

RegionDelegate *DeepEdgePairs::polygons (db::Coord e) const
{
  db::DeepLayer new_layer = deep_layer ().derived ();
  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (db::ShapeIterator::EdgePairs); ! s.at_end (); ++s) {
      db::Polygon poly = s->edge_pair ().normalized ().to_polygon (e);
      if (poly.vertices () >= 3) {
        output.insert (db::PolygonRef (poly, layout.shape_repository ()));
      }
    }
  }

  return new db::DeepRegion (new_layer);
}

EdgesDelegate *DeepEdgePairs::generic_edges (bool first, bool second) const
{
  db::DeepLayer new_layer = deep_layer ().derived ();
  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (db::ShapeIterator::EdgePairs); ! s.at_end (); ++s) {
      db::EdgePair ep = s->edge_pair ();
      if (first) {
        output.insert (ep.first ());
      }
      if (second) {
        output.insert (ep.second ());
      }
    }
  }

  return new db::DeepEdges (new_layer);
}

EdgesDelegate *DeepEdgePairs::edges () const
{
  return generic_edges (true, true);
}

EdgesDelegate *DeepEdgePairs::first_edges () const
{
  return generic_edges (true, false);
}

EdgesDelegate *DeepEdgePairs::second_edges () const
{
  return generic_edges (false, true);
}

EdgePairsDelegate *DeepEdgePairs::in (const EdgePairs &other, bool invert) const
{
  //  TODO: implement
  return AsIfFlatEdgePairs::in (other, invert);
}

bool DeepEdgePairs::equals (const EdgePairs &other) const
{
  const DeepEdgePairs *other_delegate = dynamic_cast<const DeepEdgePairs *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()
      && other_delegate->deep_layer ().layer () == deep_layer ().layer ()) {
    return true;
  } else {
    return AsIfFlatEdgePairs::equals (other);
  }
}

bool DeepEdgePairs::less (const EdgePairs &other) const
{
  const DeepEdgePairs *other_delegate = dynamic_cast<const DeepEdgePairs *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()) {
    return other_delegate->deep_layer ().layer () < deep_layer ().layer ();
  } else {
    return AsIfFlatEdgePairs::less (other);
  }
}

void DeepEdgePairs::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  deep_layer ().insert_into (layout, into_cell, into_layer);
}

void DeepEdgePairs::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  deep_layer ().insert_into_as_polygons (layout, into_cell, into_layer, enl);
}

}

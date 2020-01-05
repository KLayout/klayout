
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


#ifndef HDR_dbDeepEdgePairs
#define HDR_dbDeepEdgePairs

#include "dbCommon.h"

#include "dbAsIfFlatEdgePairs.h"
#include "dbDeepShapeStore.h"
#include "dbEdgePairs.h"

namespace db {

/**
 *  @brief Provides hierarchical edges implementation
 */
class DB_PUBLIC DeepEdgePairs
  : public db::AsIfFlatEdgePairs
{
public:
  DeepEdgePairs ();
  DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss);
  DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans);

  DeepEdgePairs (const DeepEdgePairs &other);
  DeepEdgePairs (const DeepLayer &dl);

  virtual ~DeepEdgePairs ();

  EdgePairsDelegate *clone () const;

  virtual EdgePairsIteratorDelegate *begin () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;

  virtual size_t size () const;
  virtual std::string to_string (size_t) const;
  virtual Box bbox () const;
  virtual bool empty () const;
  virtual const db::EdgePair *nth (size_t n) const;
  virtual bool has_valid_edge_pairs () const;
  virtual const db::RecursiveShapeIterator *iter () const;

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &filter);
  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &) const;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other);
  virtual EdgePairsDelegate *add (const EdgePairs &other) const;

  virtual RegionDelegate *polygons (db::Coord e) const;
  virtual EdgesDelegate *edges () const;
  virtual EdgesDelegate *first_edges () const;
  virtual EdgesDelegate *second_edges () const;

  virtual EdgePairsDelegate *in (const EdgePairs &, bool) const;

  virtual bool equals (const EdgePairs &other) const;
  virtual bool less (const EdgePairs &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const;

  const DeepLayer &deep_layer () const
  {
    return m_deep_layer;
  }

  DeepLayer &deep_layer ()
  {
    return m_deep_layer;
  }

private:
  DeepEdgePairs &operator= (const DeepEdgePairs &other);

  DeepLayer m_deep_layer;

  void init ();
  EdgesDelegate *generic_edges (bool first, bool second) const;
};

}

#endif



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


#ifndef HDR_dbEmptyEdgePairs
#define HDR_dbEmptyEdgePairs

#include "dbCommon.h"

#include "dbEdgePairsDelegate.h"
#include "dbRecursiveShapeIterator.h"

namespace db {

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EmptyEdgePairs
  : public EdgePairsDelegate
{
public:
  EmptyEdgePairs ();
  EmptyEdgePairs (const EmptyEdgePairs &other);

  virtual EdgePairsDelegate *clone () const;

  virtual std::string to_string (size_t) const { return std::string (); }

  virtual EdgePairsIteratorDelegate *begin () const { return 0; }
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const { return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }

  virtual Box bbox () const { return Box (); }

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &) { return this; }
  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &) const { return new EmptyEdgePairs (); }
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> filtered_pair (const EdgePairFilterBase &) const { return std::make_pair (new EmptyEdgePairs (), new EmptyEdgePairs ()); }
  virtual EdgePairsDelegate *process_in_place (const EdgePairProcessorBase &) { return this; }
  virtual EdgePairsDelegate *processed (const EdgePairProcessorBase &) const { return new EmptyEdgePairs (); }
  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const;

  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual EdgesDelegate *pull_interacting (const Edges &) const;
  virtual EdgePairsDelegate *selected_interacting (const Region &, size_t, size_t) const { return new EmptyEdgePairs (); }
  virtual EdgePairsDelegate *selected_not_interacting (const Region &, size_t, size_t) const { return new EmptyEdgePairs (); }
  virtual EdgePairsDelegate *selected_interacting (const Edges &, size_t, size_t) const { return new EmptyEdgePairs (); }
  virtual EdgePairsDelegate *selected_not_interacting (const Edges &, size_t, size_t) const { return new EmptyEdgePairs (); }
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_interacting_pair (const Region &, size_t, size_t) const { return std::make_pair (new EmptyEdgePairs (), new EmptyEdgePairs ()); }
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_interacting_pair (const Edges &, size_t, size_t) const { return std::make_pair (new EmptyEdgePairs (), new EmptyEdgePairs ()); }

  virtual EdgePairsDelegate *selected_outside (const Region &) const { return new EmptyEdgePairs (); }
  virtual EdgePairsDelegate *selected_not_outside (const Region &) const { return new EmptyEdgePairs (); }
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_outside_pair (const Region &) const { return std::make_pair (new EmptyEdgePairs (), new EmptyEdgePairs ()); }
  virtual EdgePairsDelegate *selected_inside (const Region &) const { return new EmptyEdgePairs (); }
  virtual EdgePairsDelegate *selected_not_inside (const Region &) const { return new EmptyEdgePairs (); }
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_inside_pair (const Region &) const { return std::make_pair (new EmptyEdgePairs (), new EmptyEdgePairs ()); }

  virtual RegionDelegate *polygons (db::Coord e) const;
  virtual EdgesDelegate *edges () const;
  virtual EdgesDelegate *first_edges () const;
  virtual EdgesDelegate *second_edges () const;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other);
  virtual EdgePairsDelegate *add (const EdgePairs &other) const;

  virtual EdgePairsDelegate *in (const EdgePairs &, bool) const { return new EmptyEdgePairs (); }

  virtual const db::EdgePair *nth (size_t) const { tl_assert (false); }
  virtual db::properties_id_type nth_prop_id (size_t) const { tl_assert (false); }
  virtual bool has_valid_edge_pairs () const { return true; }

  virtual const db::RecursiveShapeIterator *iter () const { return 0; }
  virtual void apply_property_translator (const db::PropertiesTranslator &) { }

  virtual bool equals (const EdgePairs &other) const;
  virtual bool less (const EdgePairs &other) const;

  virtual void insert_into (Layout *, db::cell_index_type, unsigned int) const { }
  virtual void insert_into_as_polygons (Layout *, db::cell_index_type, unsigned int, db::Coord) const { }

private:
  EmptyEdgePairs &operator= (const EmptyEdgePairs &other);
};

}

#endif


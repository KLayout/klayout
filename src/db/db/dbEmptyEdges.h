
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


#ifndef HDR_dbEmptyEdges
#define HDR_dbEmptyEdges

#include "dbCommon.h"
#include "dbEdgesDelegate.h"
#include "dbRecursiveShapeIterator.h"

namespace db {

/**
 *  @brief An empty Edges
 */
class DB_PUBLIC EmptyEdges
  : public EdgesDelegate
{
public:
  EmptyEdges ();
  virtual ~EmptyEdges ();

  EmptyEdges (const EmptyEdges &other);
  EdgesDelegate *clone () const;

  virtual EdgesIteratorDelegate *begin () const { return 0; }
  virtual EdgesIteratorDelegate *begin_merged () const { return 0; }

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const { return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ()); }
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const { return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }
  virtual std::string to_string (size_t) const { return std::string (); }
  virtual bool is_merged () const { return true; }
  virtual distance_type length (const db::Box &) const { return 0; }
  virtual Box bbox () const { return db::Box (); }

  virtual EdgePairsDelegate *width_check (db::Coord, const db::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *space_check (db::Coord, const db::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *enclosing_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *overlap_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *separation_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *inside_check (const Edges &, db::Coord, const db::EdgesCheckOptions &) const;

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &) { return this; }
  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &) { return this; }
  virtual EdgesDelegate *processed (const EdgeProcessorBase &) const { return new EmptyEdges (); }
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &) const;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &) const;

  virtual EdgesDelegate *merged_in_place () { return this; }
  virtual EdgesDelegate *merged () const { return new EmptyEdges (); }

  virtual EdgesDelegate *and_with (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *not_with (const Edges &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Edges &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }
  virtual EdgesDelegate *and_with (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *not_with (const Region &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Region &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }
  virtual EdgesDelegate *xor_with (const Edges &other) const;
  virtual EdgesDelegate *or_with (const Edges &other) const;
  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;
  virtual EdgesDelegate *intersections (const Edges &) const { return new EmptyEdges (); }

  virtual RegionDelegate *extended (coord_type, coord_type, coord_type, coord_type, bool) const;

  virtual EdgesDelegate *inside_part (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *outside_part (const Region &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> inside_outside_part_pair (const Region &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }

  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual EdgesDelegate *pull_interacting (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_interacting (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_interacting (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_interacting (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_interacting (const Region &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair (const Region &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair (const Edges &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }

  virtual EdgesDelegate *selected_outside (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_outside (const Region &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_outside_pair (const Region &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }
  virtual EdgesDelegate *selected_inside (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_inside (const Region &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_inside_pair (const Region &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }
  virtual EdgesDelegate *selected_outside (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_outside (const Edges &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_outside_pair (const Edges &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }
  virtual EdgesDelegate *selected_inside (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_inside (const Edges &) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_inside_pair (const Edges &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }

  virtual EdgesDelegate *in (const Edges &, bool) const { return new EmptyEdges (); }
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> in_and_out (const Edges &) const { return std::make_pair (new EmptyEdges (), new EmptyEdges ()); }

  virtual const db::Edge *nth (size_t) const { tl_assert (false); }
  virtual bool has_valid_edges () const { return true; }
  virtual bool has_valid_merged_edges () const { return true; }

  virtual const db::RecursiveShapeIterator *iter () const { return 0; }
  virtual void apply_property_translator (const db::PropertiesTranslator &) { }
  virtual db::PropertiesRepository *properties_repository () { return 0; }
  virtual const db::PropertiesRepository *properties_repository () const { return 0; }

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual void insert_into (Layout *, db::cell_index_type, unsigned int) const { }

private:
  EmptyEdges &operator= (const EmptyEdges &other);
};

}  // namespace db

#endif

